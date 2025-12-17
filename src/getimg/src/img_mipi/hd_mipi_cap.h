/**
 * @file hd_mipi_cap.h
 * @author Luke
 * @brief v4l2视频采集程序
 * @version 0.2
 * @date 2025-06-05
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HD_MIPI_CAP_H
#define HD_MIPI_CAP_H

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include <linux/videodev2.h>
#include <sys/select.h>
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif
#define GET_IMG_UYVY  (1)
/* Exported constants --------------------------------------------------------*/
#define HD_MAX_PATH_LEN 255

/* Exported types ------------------------------------------------------------*/
typedef struct HDMipiDevState HDMipiDevState;
typedef struct HDMipiBuffer HDMipiBuffer;

/**
 * @brief 设备参数
 * 设置参数后不一定生效，需要根据 HDImage 返回实际参数大小为准
 * @note 接口不正常时需返回黑色图像，请设置 width、height、stride 为实际值
 */
typedef struct HDDeviceParam {
	char *dev_name; ///< 设备名，mipi作设备名，local作图像目录
	char *dev_id;	  ///< 设备标识
	size_t width;		  ///< 宽度
	size_t height;		  ///< 高度
	size_t stride;		  ///< 步长
	float width_fov;	  ///< 宽度方向视场角
	float height_fov;	  ///< 高度方向视场角
	size_t depth;		  ///< 位深
	float cam_instl_pitch;	///< 相机安装角
	float cam_instl_yaw;	///< 相机安装角
	float cam_instl_roll;	///< 相机安装叫
} HDDeviceParam;
/**
 * @struct HDMipiDevState
 * @brief MIPI视频设备状态结构体
 *
 * 用于描述一个MIPI摄像头设备的所有运行时状态。
 */
typedef struct HDMipiDevState {
	int fd;                              ///< 设备文件描述符
	int num;// 缓冲区数
	int state;// 图像类型
	unsigned int width;		  ///< 宽度
	unsigned int height;		  ///< 高度
	bool is_error;                        ///< 设备是否错误，如果错误，则接口只会传出黑色图像
	HDDeviceParam param;				 ///< 设备参数
	struct HDBufferInfo *buffers_info;   ///< 申请的所有buffer信息
	struct v4l2_format format;           ///< 当前设备格式
	struct v4l2_buffer cur_buffer;       ///< 当前buffer信息
} HDMipiDevState;
/**
 * @brief 红外/可见光图像数据
 */
typedef struct HDImage {
	AVBufferRef *avbuffer_ref; ///< 数据引用计数
	uint8_t *data;			   ///< 数据指针
	HDDeviceParam *param;	   ///< 图像属性
	size_t idx;				   ///< 图像序号(设计为自增)
	void *hd_buf;			   ///< 各个采集类型私有数据
	size_t frame_id;		   ///< 自定义ID
} HDImage;

/* Exported functions --------------------------------------------------------*/
/**
 * @brief 打开 v4l2 video 设备
 * 
 * @param dev_name 设备名, 如 /dev/video0
 * @return struct HDMipiDevState* 
 */
HDMipiDevState *hd_open_mipi_video(HDDeviceParam *param);

/**
 * @brief 启动视频流线程，不断更新最新的一帧数据
 * 
 * @param state video 设备
 * @return int 
 */
int hd_start_mipi_video(HDMipiDevState *state);

/**
 * @brief 获取最新的一帧数据，如果没有数据，则hd_image->data为 NULL
 * 
 * @param state video 设备
 */
void hd_get_mipi_video_frame(HDMipiDevState *state, HDImage *hd_image);

/**
 * @brief 释放MIPI帧缓冲区
 * 
 * @param state  video 设备状态
 * @param hd_buf  MIPI缓冲区指针
 */
void hd_free_mipi_frame(HDMipiDevState *state, HDMipiBuffer *hd_buf);

/**
 * @brief 停止视频流
*/
int hd_stop_mipi_video(HDMipiDevState *state);

/**
 * @brief 清除设备资源
 * 
 * @param state 
 * @return int 
 */
int hd_clear_mipi_dev(HDMipiDevState *state);

#ifdef __cplusplus
}
#endif

#endif // HD_VIDEO_CAP_H
