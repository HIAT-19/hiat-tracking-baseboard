#include "img_local.h"

/**
 * XDMA 218读取函数
 * 从XDMA设备读取图像数据到缓冲区
 * 
 * @param ctx 设备上下文指针，指向ImgXdmaGetOpt结构
 * @param img 输出参数，指向存储图像数据的缓冲区指针
 * @return 成功返回0，失败返回-1
 */
static int localRead(void *ctx, unsigned char** img)
{
    // 转换上下文指针为ImgXdmaGetOpt结构
	ImgLocalGetOpt *p = (ImgLocalGetOpt *)(ctx);
	int ret = 0;
	int ct_k;
	Task task = {0};              // 任务结构体，用于队列操作
	uint64_t exp;
    static unsigned long long num = 0;
    FILE* fp = NULL;
    char cmdstr[100]={0};

    if (num == 0) {
        num = (unsigned long long)p->dev->id;  // 第一次调用时赋值
    }
    ret = epoll_wait(p->dev->epoll_fd, &(p->dev->ev), 1, -1);
    if (ret < 0) {
        perror("epoll_wait");
        return -1;
    }

    if (read(p->dev->timer_fd, &exp, sizeof(exp)) != sizeof(exp)) {
        perror("read timer_fd");
        return -2;
    }

    if (exp > 0) {
        while(fp == NULL){
            sprintf(cmdstr,p->dev->dev_name,num);
            fp = fopen(cmdstr,"r+b");
            num++;
        }

        // 将任务放入队列
		ct_k = queue_push(&(p->dev->queue), task, 1);
		if(ct_k == -1){
            // 入队失败，空间满了
			printf("localRead queue_push ct_k no\n");
            fclose(fp);
            fp = NULL;
            return -3;
		}else{
            ret = (int)fread(p->dev->queue.Tdata.buffer[ct_k], 1, p->dev->width * p->dev->height * 2,fp);
            if (ret == -1){
                printf("localRead read err\n");
                fclose(fp);
                fp = NULL;
                return -4;
            }
            fclose(fp);
            fp = NULL;

            // 设置任务参数
			task.free_buf_id = ct_k;                  // 缓冲区ID
			*img = p->dev->queue.Tdata.buffer[ct_k];  // 输出图像数据指针
            
			queue_push(&(p->dev->queue), task, 0);
		} 
    }
   
	return 0;  // 成功返回
}

static int localClean(void *ctx)
{
    int i;
    ImgLocalGetOpt *devopt = (ImgLocalGetOpt *)(ctx);
    // 清理资源（需要实现对应的清理函数）
    if(devopt){
        if(devopt->dev){
            if (devopt->dev->timer_fd >= 0) close(devopt->dev->timer_fd);
            if (devopt->dev->epoll_fd >= 0) close(devopt->dev->epoll_fd);
            if (devopt->dev->img) {
                for (i = 0; i < devopt->dev->bufnum; i++) {
                    if (devopt->dev->img[i]) free(devopt->dev->img[i]);
                }
                free(devopt->dev->img);
            }
            if(devopt->dev->queue.tasks) queue_destroy(&(devopt->dev->queue));
            free(devopt->dev);
        }
        free(devopt);
    }
	return 0;
}

static int localPut(void *ctx)
{
	ImgLocalGetOpt *p = (ImgLocalGetOpt *)(ctx);

    if(queue_kong(&(p->dev->queue)) != 0 ){
        queue_pop(&(p->dev->queue), 1);
	    queue_pop(&(p->dev->queue), 0);
    }
	
	return 0;
}

/**
 * XDMA设备初始化
 * @param ctx 设备上下文指针（输入输出参数）
 * @return 成功返回0，失败返回负的错误码
 */
int ImgGetLocalInit(void **ctx)
{
    int i;

    size_t devidlen = 0;
    size_t devnamelen = 0;
    struct itimerspec ts;

    // 参数检查
    if (!ctx) {
        printf("Error: Invalid context pointer\n");
        return -1;
    }

    ImgGetLocalInitData *p = (ImgGetLocalInitData *)(*ctx);
    if (!p) {
        printf("Error: Invalid init data\n");
        return -2;
    }

    // 分配设备操作结构体
    ImgLocalGetOpt *devopt = (ImgLocalGetOpt*)malloc(sizeof(ImgLocalGetOpt));
    if (!devopt) {
        printf("Error: Failed to allocate ImgLocalGetOpt\n");
        return -3;
    }

    // 计算字符串长度
    devidlen = strlen(p->dev_id) + 1;
    devnamelen = strlen(p->dev_name) + 1;

    // 分配设备数据内存
    devopt->dev = (ImgGetLocalInitData*)malloc(sizeof(ImgGetLocalInitData) + devidlen + devnamelen);
    if (!devopt->dev) {
        printf("Error: Failed to allocate device data\n");
        goto cleanupLocalInit;
    }

    // 初始化设备参数
    devopt->dev->width = p->width;
    devopt->dev->height = p->height;
    devopt->dev->dev_id = (char *)(devopt->dev + 1);
    devopt->dev->dev_name = (char *)(devopt->dev + 1 + devidlen);
    devopt->dev->id = p->id;
    devopt->dev->gpio = p->gpio;
    devopt->dev->bufnum = p->bufnum;

    // 复制设备ID和名称
    memcpy(devopt->dev->dev_id, p->dev_id, devidlen);
    memcpy(devopt->dev->dev_name, p->dev_name, devnamelen);

    // 创建定时器
    devopt->dev->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (devopt->dev->timer_fd < 0) {
        fprintf(stderr, "timerfd_create failed: %s\n", strerror(errno));
        // 返回错误码或进行错误处理
        goto cleanupLocalInit; // 或其他错误处理
    }
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = p->gpio * 1000000L;  // 注意：p应该是devopt
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = p->gpio * 1000000L;
    if (timerfd_settime(devopt->dev->timer_fd, 0, &ts, NULL) < 0) {
        fprintf(stderr, "timerfd_settime failed: %s\n", strerror(errno));
        goto cleanupLocalInit;
    }
    // 创建epoll
    devopt->dev->epoll_fd = epoll_create1(0);
    if (devopt->dev->epoll_fd < 0) {
        fprintf(stderr, "epoll_create1 failed: %s\n", strerror(errno));
        goto cleanupLocalInit;
    }
    devopt->dev->ev.events = EPOLLIN;
    devopt->dev->ev.data.fd = devopt->dev->timer_fd;
    if (epoll_ctl(devopt->dev->epoll_fd, EPOLL_CTL_ADD, devopt->dev->timer_fd, &(devopt->dev->ev)) < 0) {
        fprintf(stderr, "epoll_ctl add timer_fd failed: %s\n", strerror(errno));
        goto cleanupLocalInit;
    }
    // 初始化队列
    if (queue_init(&(devopt->dev->queue), devopt->dev->bufnum) != 0) {
        printf("Error: Failed to initialize queue\n");
        goto cleanupLocalInit;
    }

    // 分配图像缓冲区
    devopt->dev->img = (unsigned char**)malloc((unsigned int)(devopt->dev->bufnum) * sizeof(char*));
    if (!devopt->dev->img) {
        printf("Error: Failed to allocate image buffer array\n");
        goto cleanupLocalInit;
    }

    for (i = 0; i < devopt->dev->bufnum; i++) {
        devopt->dev->img[i] = (unsigned char*)malloc(devopt->dev->width * devopt->dev->height * 2);
        if (!devopt->dev->img[i]) {
            printf("Error: Failed to allocate image buffer %d\n", i);
            goto cleanupLocalInit;
        }
        memset(devopt->dev->img[i], 0, devopt->dev->width * devopt->dev->height * 2);
        devopt->dev->queue.Tdata.buffer[i] = devopt->dev->img[i];
    }
    
    // 设置操作函数
    devopt->read_frame = localRead;
    devopt->put_frame = localPut;
    devopt->clean_frame = localClean;
    // 返回设备句柄
    *ctx = (void*)devopt;
    return 0;

cleanupLocalInit:
    // 清理资源（需要实现对应的清理函数）
    if(devopt){
        if(devopt->dev){
            if (devopt->dev->timer_fd >= 0) close(devopt->dev->timer_fd);
            if (devopt->dev->epoll_fd >= 0) close(devopt->dev->epoll_fd);
            if (devopt->dev->img) {
                for (i = 0; i < devopt->dev->bufnum; i++) {
                    if (devopt->dev->img[i]) free(devopt->dev->img[i]);
                }
                free(devopt->dev->img);
            }
            if(devopt->dev->queue.tasks) queue_destroy(&(devopt->dev->queue));
            free(devopt->dev);
        }
        free(devopt);
    }
    return -5;
}
