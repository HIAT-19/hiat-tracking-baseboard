#include "img_mipi.h"
#include "queue.h"

static int mipiRead(void *ctx, unsigned char** img)
{
    // 转换上下文指针为ImgXdmaGetOpt结构
	ImgMipiGetOpt *p = static_cast<ImgMipiGetOpt*>(ctx);
    HDMipiDevState *mipi_dev = static_cast<HDMipiDevState*>(p->dev->devData);
	int ret = 0;
	int ct_k;
	Task task;              // 任务结构体，用于队列操作
	
    // 使用poll等待GPIO事件，超时时间20ms
	ret = poll(p->dev->fds, 1, 20); // 等待20ms超时
	if (ret <= 0){
        // 超时或错误，无数据可读
		printf("mipiRead poll timeout\n");
		return -1;
	}
		
    // 将任务放入队列
    ct_k = queue_push(&(p->dev->queue), task, 1);
    if(ct_k == -1){
        // 入队失败，空间满了
        printf("mipiRead queue_push ct_k no\n");
        return -1;
    }else{
        // 从FPGA DDR读取图像数据到队列缓冲区
        hd_get_mipi_video_frame(mipi_dev, p->dev->hd_image[ct_k]);

        if (p->dev->hd_image[ct_k]->data != NULL) {
            fprintf(stdout, "Get mipi frame success\n");
        }
        else {
            fprintf(stdout, "Get mipi frame NULL\n");
            //std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return -1;
        }
        
        
        // 设置任务参数
        task.free_buf_id = ct_k;                  // 缓冲区ID
        *img = p->dev->queue.Tdata.buffer[ct_k];  // 输出图像数据指针
        
        queue_push(&(p->dev->queue), task, 0);
    }

	return 0;  // 成功返回
}

static int mipiClean(void *ctx)
{
    int i;
    ImgMipiGetOpt *devopt = static_cast<ImgMipiGetOpt*>(ctx);
    HDMipiDevState *mipi_dev = static_cast<HDMipiDevState*>(devopt->dev->devData);
    // Release mipi dev resources
    hd_stop_mipi_video(mipi_dev);

    if(devopt){
        if(devopt->dev){
            if (mipi_dev)  hd_clear_mipi_dev(mipi_dev);
            if (devopt->dev->hd_image) {
                for (i = 0; i < devopt->dev->bufnum; i++) {
                    if (devopt->dev->hd_image[i]) free(devopt->dev->hd_image[i]);
                }
                free(devopt->dev->hd_image);
            }
            if(devopt->dev->queue.tasks) queue_destroy(&(devopt->dev->queue));
            free(devopt->dev);
        }
        free(devopt);
    }

	return 0;
}

static int mipiPut(void *ctx)
{
    ImgMipiGetOpt *p = static_cast<ImgMipiGetOpt*>(ctx);
    HDMipiDevState *mipi_dev = static_cast<HDMipiDevState*>(p->dev->devData);
    Task task;              // 任务结构体，用于队列操作

    if(queue_kong(&(p->dev->queue)) != 0 ){
        task = queue_pop(&(p->dev->queue), 1);
        hd_free_mipi_frame(mipi_dev, static_cast<HDMipiBuffer*>(p->dev->hd_image[task.free_buf_id]->hd_buf));
	    task = queue_pop(&(p->dev->queue), 0);
    }
	
	return 0;
}

/**
 * XDMA设备初始化
 * @param ctx 设备上下文指针（输入输出参数）
 * @return 成功返回0，失败返回负的错误码
 */
int ImgGetMipiInit(void **ctx)
{
    int i;
    size_t devidlen = 0;
    size_t devnamelen = 0;
    // Init dev param
    HDDeviceParam dev_param{};
    HDMipiDevState *mipi_dev = nullptr;

    // 参数检查
    if (!ctx) {
        printf("ImgGetMipiInit Error: Invalid context pointer\n");
        return -1;
    }

    ImgGetMipiInitData *p = static_cast<ImgGetMipiInitData*>(*ctx);
    if (!p) {
        printf("ImgGetMipiInit Error: Invalid init data\n");
        return -2;
    }

    // 分配设备操作结构体
    ImgMipiGetOpt *devopt = static_cast<ImgMipiGetOpt*>(malloc(sizeof(ImgMipiGetOpt)));
    if (!devopt) {
        printf("ImgGetMipiInit Error: Failed to allocate ImgXdmaGetOpt\n");
        return -3;
    }

    // 计算字符串长度
    devidlen = strlen(p->dev_id) + 1;
    devnamelen = strlen(p->dev_name) + 1;

    // 分配设备数据内存
    devopt->dev = static_cast<ImgGetMipiInitData*>(malloc(sizeof(ImgGetMipiInitData) + devidlen + devnamelen));
    if (!devopt->dev) {
        printf("ImgGetMipiInit Error: Failed to allocate device data\n");
        goto cleanupMipiInit;
    }

    // 初始化设备参数
    devopt->dev->width = p->width;
    devopt->dev->height = p->height;
    devopt->dev->dev_id = reinterpret_cast<char*>(devopt->dev + 1);
    devopt->dev->dev_name = reinterpret_cast<char*>(devopt->dev + 1 + devidlen);
    devopt->dev->id = p->id;
    devopt->dev->bufnum = p->bufnum;

    // 复制设备ID和名称
    memcpy(devopt->dev->dev_id, p->dev_id, devidlen);
    memcpy(devopt->dev->dev_name, p->dev_name, devnamelen);
    memcpy(dev_param.dev_id, p->dev_id, devidlen);
    memcpy(dev_param.dev_name, p->dev_name, devnamelen);

    // Open mipi device 
    mipi_dev = hd_open_mipi_video(&dev_param);
    if (mipi_dev == NULL)  {
        fprintf(stderr, "ImgGetMipiInit Failed to open mipi device: %s\n", dev_param.dev_name);
        goto cleanupMipiInit;
    }

    // Start mipi stream
    mipi_dev->num = p->bufnum;
    mipi_dev->state = p->id;
    mipi_dev->width = p->width;
    mipi_dev->height = p->height;
    

    // 初始化队列
    if (queue_init(&(devopt->dev->queue), devopt->dev->bufnum) != 0) {
        printf("ImgGetMipiInit Error: Failed to initialize queue\n");
        goto cleanupMipiInit;
    }
    // Frame cap loop
    // 分配图像缓冲区
    devopt->dev->hd_image = static_cast<HDImage**>(malloc(static_cast<unsigned int>(devopt->dev->bufnum) * sizeof(HDImage*)));
    if (!devopt->dev->hd_image) {
        printf("ImgGetMipiInit Error: Failed to allocate image buffer array\n");
        goto cleanupMipiInit;
    }
    for (i = 0; i < devopt->dev->bufnum; i++) {
        devopt->dev->hd_image[i] = static_cast<HDImage*>(malloc(sizeof(HDImage)));
        if (!devopt->dev->hd_image[i]) {
            printf("ImgGetMipiInit Error: Failed to allocate image buffer %d\n", i);
            goto cleanupMipiInit;
        }
        memset(devopt->dev->hd_image[i], 0, sizeof(HDImage));
        devopt->dev->queue.Tdata.buffer[i] = devopt->dev->hd_image[i]->data;
    }

    // 设置pollfd
    devopt->dev->fds[0].fd = mipi_dev->fd;
    devopt->dev->fds[0].events = POLLIN;

    devopt->dev->devData = mipi_dev;
    
    hd_start_mipi_video(mipi_dev);
    // // 设置操作函数
    devopt->read_frame = mipiRead;
    devopt->put_frame = mipiPut;
    devopt->clean_frame = mipiClean;
    // // 返回设备句柄
    *ctx = static_cast<void*>(devopt);
    return 0;

cleanupMipiInit:
    // 清理资源（需要实现对应的清理函数）
    if(devopt){
        if(devopt->dev){
            if (mipi_dev)  hd_clear_mipi_dev(mipi_dev);
            if (devopt->dev->hd_image) {
                for (i = 0; i < devopt->dev->bufnum; i++) {
                    if (devopt->dev->hd_image[i]) free(devopt->dev->hd_image[i]);
                }
                free(devopt->dev->hd_image);
            }
            if(devopt->dev->queue.tasks) queue_destroy(&(devopt->dev->queue));
            free(devopt->dev);
        }
        free(devopt);
    }
    return -5;
}
