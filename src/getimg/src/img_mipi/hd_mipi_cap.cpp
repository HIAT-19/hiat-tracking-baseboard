/**
 * @file hd_mipi_cap.c
 * @brief MIPI摄像头图像采集
 * @author Luke
 * @date 2025-06-04
 * @note 暂时不支持多planes
 */
/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <linux/videodev2.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include "hd_mipi_cap.h"

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

/* Private constants ---------------------------------------------------------*/

enum { HD_VIDEO_BUFFER_CNT = 30 };

/* Private types -------------------------------------------------------------*/
/**
 * @typedef plane_addr
 * @brief 每一个plane起始地址的指针类型
 */
typedef void *plane_addr; ///< 每一个plane起始地址



/**
 * @brief MIPI 设备帧数据
 */
typedef struct HDMipiBuffer {
	uint32_t v4l2_buf_idx;
} HDMipiBuffer;

/**
 * @struct HDBufferInfo
 * @brief 每个向内核申请的v4l2buffer的信息
 */
struct HDBufferInfo {
	struct v4l2_buffer *buffer;      ///< v4l2 buffer结构体指针
	struct v4l2_plane *planes;       ///< 多plane信息指针
	plane_addr *planes_addr;         ///< 多plane起始地址指针
};

/* Private constants ---------------------------------------------------------*/

/* Private values ------------------------------------------------------------*/

/* Private functions declaration ---------------------------------------------*/

static void print_video_format(const struct v4l2_format *fmt);

struct HDMipiDevState *hd_state_init();

int check_frame_header(HDImage *hd_image);

/* Exported functions definition ---------------------------------------------*/

/**
 * @brief 打开MIPI视频设备
 * @param param 设备参数
 * @return 设备状态指针，失败返回NULL
 */
struct HDMipiDevState *hd_open_mipi_video(HDDeviceParam *param)
{
	if (param->dev_name == NULL) {
		return NULL;
	}
	struct HDMipiDevState *state = NULL;
	int fd = -1;

	state = hd_state_init();
	if (!state) {
		goto cleanup;
	}

	memcpy(&state->param, param, sizeof(*param));

	// Open as non-blocking mode
	state->fd = open(param->dev_name, O_RDWR | O_NONBLOCK);
	fd = state->fd;
	if (fd < 0) {
		perror("Cannot open device file");
		state->is_error = true;
		goto cleanup;
	}

	return state;

cleanup:
	if (state) {
		hd_clear_mipi_dev(state);
	}
	return NULL;
}

/**
 * @brief 启动MIPI视频流
 * 
 * @param state 设备状态指针
 * @return 0成功，-1失败
 */
int hd_start_mipi_video(HDMipiDevState *state)
{
	struct v4l2_capability capability;
	struct v4l2_requestbuffers requestbuffers;
	struct v4l2_format *format = NULL;
	struct HDBufferInfo *buffers_info = NULL;
	size_t num_planes = 0;
	int fd = state->fd;
	int state1 = state->state;
	int num = state->num;
	unsigned int width = state->width;
	unsigned int height = state->height;


	// Query capability
	if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
		perror("Cannot query video capability");
		goto cleanup;
	}

	// Get video format
	format = &state->format;
	format->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if (ioctl(fd, VIDIOC_G_FMT, format) < 0) {
		perror("Failed to query video format");
		goto cleanup;
	}

	// Set video format
	if(state1 == GET_IMG_UYVY){
		format->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_UYVY;
	}
	
	format->fmt.pix.width = width;
	format->fmt.pix.height = height;
	if (ioctl(fd, VIDIOC_S_FMT, format) < 0) {
		perror("Failed to set video format");
		goto cleanup;
	}

	// Check video format Result
	if (ioctl(fd, VIDIOC_G_FMT, format) < 0) {
		perror("Failed to query video format");
		goto cleanup;
	}
	print_video_format(format);

	// Request kernel buffers
	requestbuffers.count = num;
	requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	requestbuffers.memory = V4L2_MEMORY_MMAP;//V4L2_MEMORY_DMABUF
	if (ioctl(fd, VIDIOC_REQBUFS, &requestbuffers) < 0) {
		perror("Failed to request buffers");
		goto cleanup;
	}

	state->buffers_info = static_cast<struct HDBufferInfo *>(malloc(
		requestbuffers.count * sizeof(struct HDBufferInfo)));
	if (!state->buffers_info) goto cleanup;

	buffers_info = state->buffers_info;
	num_planes = state->format.fmt.pix_mp.num_planes;
	for (size_t i = 0; i < requestbuffers.count; ++i) {
		buffers_info[i].buffer = static_cast<struct v4l2_buffer *>(calloc(
			1, sizeof(struct v4l2_buffer)));
		if (!buffers_info[i].buffer) goto cleanup;

		buffers_info[i].planes = static_cast<struct v4l2_plane *>(calloc(
			num_planes, sizeof(struct v4l2_plane)));
		if (!buffers_info[i].planes) goto cleanup;
		buffers_info[i].planes_addr =
			static_cast<plane_addr *>(calloc(num_planes, sizeof(plane_addr)));
		if (!buffers_info[i].planes_addr) goto cleanup;

		buffers_info[i].buffer->type =
			V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buffers_info[i].buffer->memory = V4L2_MEMORY_MMAP;
		buffers_info[i].buffer->m.planes = buffers_info[i].planes;
		buffers_info[i].buffer->length = static_cast<uint32_t> (num_planes);
		buffers_info[i].buffer->index = static_cast<uint32_t> (i);

		if (ioctl(fd, VIDIOC_QUERYBUF, buffers_info[i].buffer) < 0) {
			perror("Failed to query buffer");
			goto cleanup;
		}

		for (size_t j = 0; j < num_planes; ++j) {
			buffers_info[i].planes_addr[j] =
				mmap(NULL, buffers_info[i].planes[j].length,
				     PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				     buffers_info[i].planes[j].m.mem_offset);
			if (MAP_FAILED == (buffers_info[i].planes_addr[j])) {
				perror("Failed to mmap assigned kernel buffer for video");
				goto cleanup;
			}
		}
	}

	memcpy(&state->cur_buffer, buffers_info[0].buffer,
	       sizeof(struct v4l2_buffer));

	for (size_t i = 0; i < requestbuffers.count; ++i) {
		if (ioctl(fd, VIDIOC_QBUF, buffers_info[i].buffer) < 0) {
			perror("Failed to queue buffers");
			goto cleanup;
		}
	}

	if (0 > ioctl(fd, VIDIOC_STREAMON, &state->format.type)) {
		perror("Failed to start video stream");
		goto cleanup;
	}

	printf("*** Dev: %s. Video stream started *** \n", state->param.dev_name);
	return 0;

cleanup:
	hd_stop_mipi_video(state);

	return -1;
}

/**
 * @brief 获取最新一帧MIPI视频数据（always latest模式）
 *
 * 该函数会循环DQBUF，直到队列为空（EAGAIN），只返回最后一帧，其余帧立即QBUF回内核。
 * @param state MIPI设备状态指针
 * @return HDImage* 成功返回帧指针，失败返回NULL
 * @note TODO: 优化为内存池管理，避免频繁malloc/free
 */
void hd_get_mipi_video_frame(HDMipiDevState *state, HDImage *hd_image)
{
	if (!state || !hd_image) return;

	HDMipiBuffer *mipi_buf = nullptr;
	HDDeviceParam *param = nullptr;

	// 如果设备错误，则返回黑色图像
	if (state->is_error) {
		printf("Device is error, return black image\n");
		/// @note: 当拉流异常时，stride可能被赋值为 0
		if (state->param.stride == 0 && state->param.width == 640) {
			state->param.stride = 768;	
		}
		else if (state->param.stride == 0 && state->param.width == 2048) {
			state->param.stride = 2048;
		}
		hd_image->hd_buf = static_cast<uint8_t *>(calloc(1, state->param.stride * state->param.height));
		hd_image->data = static_cast<uint8_t *>(hd_image->hd_buf);	
		hd_image->param = &state->param;
		return;
	}

	mipi_buf = static_cast<HDMipiBuffer *>(calloc(1, sizeof(HDMipiBuffer)));
	if (!mipi_buf) return;

	uint32_t last_idx = UINT32_MAX;
	bool got_frame = false;

	while (1) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.length = state->format.fmt.pix_mp.num_planes;
		buf.m.planes = state->buffers_info[0].planes;

		if (ioctl(state->fd, VIDIOC_DQBUF, &buf) < 0) {
			if (errno == EAGAIN) break;
			perror("Failed to dequeue buffer");
			goto fail;
		}

		// 如果不是第一帧，前一帧要立即QBUF回内核
		if (got_frame) {
			struct v4l2_buffer *qbuf = state->buffers_info[last_idx].buffer;
			qbuf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			qbuf->memory = V4L2_MEMORY_MMAP;
			qbuf->length = state->format.fmt.pix_mp.num_planes;
			qbuf->m.planes = state->buffers_info[last_idx].planes;
			if (ioctl(state->fd, VIDIOC_QBUF, qbuf) < 0) {
				perror("Failed to queue buffer");
				goto fail;
			}
		}

		last_idx = buf.index;
		got_frame = true;
	}

	if (!got_frame) {
		// 没有新帧
		goto fail;
	}

	mipi_buf->v4l2_buf_idx = last_idx;

	// 2. 填充HDImage结构体
	hd_image->param = &state->param;
	param = hd_image->param;
	hd_image->hd_buf = mipi_buf;
	hd_image->data = static_cast<uint8_t *>(state->buffers_info[last_idx].planes_addr[0]);
	param->width = state->format.fmt.pix.width;
	param->height = state->format.fmt.pix.height;
	param->stride = state->format.fmt.pix_mp.plane_fmt[0].bytesperline;

	// TODO(luke): 根据实际格式设置
	hd_image->idx = 0;

	// 注意：最后一帧的QBUF应由上层在frame用完后调用hd_free_mipi_frame时完成，
	// 但在always latest模式下，frame只在本次采集周期内有效，建议在下次采集前QBUF。
	// 这里不做QBUF，交由上层管理。
	return ;

fail:
	if (mipi_buf) {
		free(mipi_buf);
	}
	hd_image->data = NULL;
}

/**
 * @brief 释放当前帧
 * @param state 设备状态指针
 * @param frame 要释放的帧指针
 */
void hd_free_mipi_frame(HDMipiDevState *state, HDMipiBuffer *hd_buf)
{
	if (hd_buf == NULL) return;

	// 如果设备错误，则直接释放黑色图像数据
	if (state->is_error) {
		free(hd_buf);
		return;
	}

	uint32_t idx = hd_buf->v4l2_buf_idx;
	struct v4l2_buffer *qbuf = state->buffers_info[idx].buffer;
	qbuf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	qbuf->memory = V4L2_MEMORY_MMAP;
	qbuf->length = state->format.fmt.pix_mp.num_planes;
	qbuf->m.planes = state->buffers_info[idx].planes;

	if (ioctl(state->fd, VIDIOC_QBUF, qbuf) < 0) {
		perror("Failed to queue buffer");
	}

	free(hd_buf);
}

int hd_stop_mipi_video(HDMipiDevState *state)
{
	if (state == NULL) {
		return 0;
	}

	// 关闭视频流
	if (0 > ioctl(state->fd, VIDIOC_STREAMOFF, &state->format.type)) {
		perror("Failed to stop video stream");
	}

	// 释放buffers
	if (state->buffers_info) {
		struct HDBufferInfo *buffers_info = state->buffers_info;
		size_t num_planes = state->format.fmt.pix_mp.num_planes;
		for (size_t i = 0; i < HD_VIDEO_BUFFER_CNT; ++i) {
			if (buffers_info[i].planes_addr) {
				for (size_t j = 0; j < num_planes; ++j) {
					if (buffers_info[i].planes_addr[j] && buffers_info[i].planes && buffers_info[i].planes[j].length > 0) {
						munmap(buffers_info[i].planes_addr[j], buffers_info[i].planes[j].length);
					}
				}
				free(buffers_info[i].planes_addr);
				buffers_info[i].planes_addr = NULL;
			}
			if (buffers_info[i].planes) {
				free(buffers_info[i].planes);
				buffers_info[i].planes = NULL;
			}
			if (buffers_info[i].buffer) {
				free(buffers_info[i].buffer);
				buffers_info[i].buffer = NULL;
			}
		}
		free(state->buffers_info);
		state->buffers_info = NULL;
	}
	// 释放内核buffer
	if (state->fd >= 0) {
		struct v4l2_requestbuffers requestbuffers;
		memset(&requestbuffers, 0, sizeof(requestbuffers));
		requestbuffers.count = 0; /**< 0 for release kernel buffer? */
		requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		requestbuffers.memory = V4L2_MEMORY_MMAP;
		if (ioctl(state->fd, VIDIOC_REQBUFS, &requestbuffers)) {
			perror("Failed to release buffers");
		}
	}

	return 0;
}

/* Private functions definition-----------------------------------------------*/
/**
 * @brief 打印视频格式信息
 * @param fmt 视频格式结构体指针
 */
static void print_video_format(const struct v4l2_format *fmt)
{
	printf("======= Current image format ====== :\n");

	printf("Type %d: ", fmt->type);
	printf("%s\n", fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ?
				     "Video Capture" :
		       fmt->type == V4L2_BUF_TYPE_VIDEO_OUTPUT ?
				     "Video Output" :
		       fmt->type == V4L2_BUF_TYPE_VBI_CAPTURE ?
				     "VBI Capture" :
		       fmt->type == V4L2_BUF_TYPE_VBI_OUTPUT ?
				     "VBI Output" :
		       fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY ?
				     "Video Overlay" :
		       fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ?
				     "multi-planar video capture stream" :
				     "Unknown");

	printf("Format Value %d: ", fmt->fmt.pix.pixelformat);

	printf("%c%c%c%c\n", fmt->fmt.pix.pixelformat,
	       fmt->fmt.pix.pixelformat >> 8, // NOLINT
	       fmt->fmt.pix.pixelformat >> 16, // NOLINT
	       fmt->fmt.pix.pixelformat >> 24); // NOLINT

	printf("Size: %dx%d\n", fmt->fmt.pix.width, fmt->fmt.pix.height);

	printf("SizeImage: %d bytes\n", fmt->fmt.pix.sizeimage);

	printf("BytesPerLine: %d\n", fmt->fmt.pix.bytesperline);

	printf("Colorspace: %d\n", fmt->fmt.pix.colorspace);

	if (fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		printf("Number of planes: %d\n", fmt->fmt.pix_mp.num_planes);
	}

	printf("\n");
}

/**
 * @brief 清理并释放MIPI设备相关资源
 * @param state 设备状态指针
 * @return 0成功
 */
int hd_clear_mipi_dev(HDMipiDevState *state)
{
	if (state == NULL) {
		return 0;
	}

	if (state->fd >= 0) close(state->fd);

	free(state);
	state = NULL;
	return 0;
}

/**
 * @brief 初始化设备状态结构体
 * @return struct HDMipiDevState* 设备状态指针，失败返回NULL
 */
struct HDMipiDevState *hd_state_init()
{
	struct HDMipiDevState *state = NULL;
	state = static_cast<struct HDMipiDevState *>(calloc(1, sizeof(*state)));
	if (NULL == state) {
		perror("Cannot allocate memory for device state");
		return NULL;
	}

	return state;
}
