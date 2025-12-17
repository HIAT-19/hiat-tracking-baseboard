#include "img_xdma.h"
#include "queue.h"
static void* mmap_control(int fd, long mapsize)
{
    void* vir_addr;
    vir_addr = mmap(0, (size_t)mapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return vir_addr;
}
// 释放映射的函数
static void unmap_control(void* addr, long mapsize)
{
    if (addr != MAP_FAILED && addr != NULL) {
        munmap(addr, (size_t)mapsize);
    }
}
// xdma218专用
static int analysis_camera(Fpgacamera218 *fpgacamera, unsigned char* control_base, int cameralen)
{
    unsigned char *buff;

    buff = (unsigned char*)(fpgacamera);
    buff[0] = *(control_base + cameralen + 1);
    buff[1] = *(control_base + cameralen + 0);
    buff[2] = *(control_base + cameralen + 3);
    buff[3] = *(control_base + cameralen + 2);
    buff[4] = *(control_base + cameralen + 5);
    buff[5] = *(control_base + cameralen + 4);
    buff[6] = *(control_base + cameralen + 7);
    buff[7] = *(control_base + cameralen + 6);
    buff[8] = *(control_base + cameralen + 11);
    buff[9] = *(control_base + cameralen + 10);
    buff[10] = *(control_base + cameralen + 9);
    buff[11] = *(control_base + cameralen + 8);
    buff[12] = *(control_base + cameralen + 15);
    buff[13] = *(control_base + cameralen + 14);
    buff[14] = *(control_base + cameralen + 13);
    buff[15] = *(control_base + cameralen + 12);
    return 0;
}
/**
 * 从FPGA DDR读取数据
 * @param c2h_dma_fd DMA文件描述符
 * @param fpga_ddr_addr FPGA DDR地址
 * @param buffer 数据缓冲区
 * @param size 数据大小
 * @return 成功读取的字节数，-1表示失败
 */
static int get_data_from_fpga_ddr(int c2h_dma_fd, unsigned int fpga_ddr_addr, unsigned char* buffer, unsigned int size)
{
    int ret;
    
    /* 定位到FPGA DDR地址 */
    ret = (int)lseek(c2h_dma_fd, fpga_ddr_addr, SEEK_SET);
    if (ret < 0) {
		printf("get_data_from_fpga_ddr lseek err\n");
        return -1;  /* lseek失败 */
    }
    
    /* 读取数据 */
    ret = (int)read(c2h_dma_fd, buffer, size);
    if (ret < 0) {
		printf("get_data_from_fpga_ddr read err\n");
        return -1;  /* read失败 */
    }

    return 0;  
}

/**
 * XDMA 218读取函数
 * 从XDMA设备读取图像数据到缓冲区
 * 
 * @param ctx 设备上下文指针，指向ImgXdmaGetOpt结构
 * @param img 输出参数，指向存储图像数据的缓冲区指针
 * @return 成功返回0，失败返回-1
 */
static int xdmaRead218(void *ctx, unsigned char** img)
{
    // 转换上下文指针为ImgXdmaGetOpt结构
	ImgXdmaGetOpt *p = (ImgXdmaGetOpt *)(ctx);
    
    // 获取XDMA 218特定设备数据
    ImgXdmaInitData218 *devdata = (ImgXdmaInitData218 *)(p->dev->devData);
	int ret = 0;
	int ct_k;
	Task task = {0};              // 任务结构体，用于队列操作
	char buff[11]= {0};     // 缓冲区，用于读取GPIO值
	
    // 使用poll等待GPIO事件，超时时间20ms
	ret = poll(p->dev->fds, 1, 20); // 等待20ms超时
	if (ret <= 0){
        // 超时或错误，无数据可读
		printf("poll timeout\n");
		return -1;
	}
		
    // 检查是否发生POLLPRI事件（GPIO上升沿）
	if (p->dev->fds[0].revents & POLLPRI)
	{
        // 重新定位到GPIO文件开始位置
		ret = (int)lseek(devdata->gpio_fd, 0, SEEK_SET);
		if (ret == -1){
            printf("xdmaRead lseek err\n");
            return -1;
        }

        // 读取GPIO值（触发poll的事件）
		ret = (int)read(devdata->gpio_fd, buff, 10);
		if (ret == -1){
            printf("xdmaRead read err\n");
            return -1;
        }

        // 解析FPGA相机寄存器数据
		analysis_camera(&(devdata->fpgacamera), p->dev->control_base, 0);
        
        // 调试信息：打印相机状态
		// printf("camera_id = %d\n", devdata->fpgacamera.camera_id);
		// printf("buffer_index = %d\n", devdata->fpgacamera.buffer_index);
		// printf("in_speed = %d\n", devdata->fpgacamera.in_speed);
		// printf("out_speed = %d\n", devdata->fpgacamera.out_speed);
		// printf("buffer_count = %d\n", devdata->fpgacamera.buffer_count);
		// printf("ddr_error_count = %d\n", devdata->fpgacamera.ddr_error_count);

		// 将任务放入队列
		ct_k = queue_push(&(p->dev->queue), task, 1);
		if(ct_k == -1){
            // 入队失败，空间满了
			printf("xdmaRead queue_push ct_k no\n");
            return -1;
		}else{
            // 从FPGA DDR读取图像数据到队列缓冲区
			ret = get_data_from_fpga_ddr(p->dev->c2h_dma_fd, 
                                 devdata->c2h_fpga_ddr_addr[devdata->fpgacamera.buffer_index], 
                                 p->dev->queue.Tdata.buffer[ct_k], 
                                 p->dev->width * p->dev->height * 2);	
            if (ret == -1){
                printf("xdmaRead get_data_from_fpga_ddr err\n");
                return -1;
            }
            
            // 设置任务参数
			task.free_buf_id = ct_k;                  // 缓冲区ID
			*img = p->dev->queue.Tdata.buffer[ct_k];  // 输出图像数据指针
            
			queue_push(&(p->dev->queue), task, 0);
		}
	}
    
	return 0;  // 成功返回
}
static int xdmaClean218(void *ctx)
{
    ImgXdmaInitData218 *pdevData = ctx;
    if (pdevData) {
        // 关闭文件描述符
        if (pdevData->gpio_fd >= 0) close(pdevData->gpio_fd);
        // 释放内存
        if (pdevData->c2h_fpga_ddr_addr) free(pdevData->c2h_fpga_ddr_addr);
        free(pdevData);
    }
    return 0;
}
static int xdmaClean(void *ctx)
{
    int i;
    ImgXdmaGetOpt *devopt = (ImgXdmaGetOpt *)(ctx);
    // 清理资源（需要实现对应的清理函数）
    if(devopt){
        if(devopt->dev){
            if (devopt->dev->control_base) unmap_control((void*)(devopt->dev->control_base), 0x40);
            if (devopt->dev->c2h_dma_fd >= 0) close(devopt->dev->c2h_dma_fd);
            if (devopt->dev->control_fd >= 0) close(devopt->dev->control_fd);
            if (devopt->dev->events0_fd >= 0) close(devopt->dev->events0_fd);
            if (devopt->dev->img) {
                for (i = 0; i < devopt->dev->bufnum; i++) {
                    if (devopt->dev->img[i]) free(devopt->dev->img[i]);
                }
                free(devopt->dev->img);
            }
            queue_destroy(&(devopt->dev->queue));
            // xdma218特定
            if (strcmp(devopt->dev->dev_id, "xdma218") == 0) {
                xdmaClean218(devopt->dev->devData);
            }  
            free(devopt->dev);
        }
        free(devopt);
    }
	return 0;
}

static int xdmaPut(void *ctx)
{
	ImgXdmaGetOpt *p = (ImgXdmaGetOpt *)(ctx);

    if(queue_kong(&(p->dev->queue)) != 0 ){
        queue_pop(&(p->dev->queue), 1);
	    queue_pop(&(p->dev->queue), 0);
    }
	
	return 0;
}

int xdmaInit218(ImgXdmaGetOpt *devopt)
{
    int ret;
    int i;
    char cmdstr[100] = {0};
    ImgXdmaInitData218* pdevData = (ImgXdmaInitData218*)malloc(sizeof(ImgXdmaInitData218));
    if (!pdevData) {
        goto cleanup;
    }
    
    // 调整控制寄存器基址（设备0有偏移）
    if (devopt->dev->id == 0) {
        devopt->dev->control_base += 16;
    }
    // GPIO配置
    memset(cmdstr,'\0',100);
    sprintf(cmdstr,"echo %d > /sys/class/gpio/export",devopt->dev->gpio);
    ret = system(cmdstr);
    if(ret != 0){
        printf("Error: GPIO export failed (gpio=%d, ret=%d)\n", devopt->dev->gpio, ret);
    }

    memset(cmdstr,'\0',100);
    sprintf(cmdstr,"echo in > /sys/class/gpio/gpio%d/direction",devopt->dev->gpio);
    ret = system(cmdstr);
    if(ret != 0){
        printf("Error: GPIO direction set failed (gpio=%d, ret=%d)\n", devopt->dev->gpio, ret);
    }

    memset(cmdstr,'\0',100);
    sprintf(cmdstr,"echo rising > /sys/class/gpio/gpio%d/edge",devopt->dev->gpio);
    ret = system(cmdstr);
    if(ret != 0){
        printf("Error: GPIO edge set failed (gpio=%d, ret=%d)\n", devopt->dev->gpio, ret);
    }

    memset(cmdstr,'\0',100);
    sprintf(cmdstr,"/sys/class/gpio/gpio%d/value",devopt->dev->gpio);
    pdevData->gpio_fd = open(cmdstr, O_RDONLY);
    if (pdevData->gpio_fd == -1) {
        printf("Error: Failed to open GPIO %d\n", devopt->dev->gpio);
        goto cleanup;
    }

    // 设置pollfd
    devopt->dev->fds[0].fd = pdevData->gpio_fd;
    devopt->dev->fds[0].events = POLLPRI;

    // 分配FPGA DDR地址数组
    pdevData->c2h_fpga_ddr_addr = (unsigned int*)malloc((unsigned int)(devopt->dev->bufnum) * sizeof(unsigned int));
    if (!pdevData->c2h_fpga_ddr_addr) {
        printf("Error: Failed to allocate FPGA DDR address array\n");
        goto cleanup;
    }

    // 设置FPGA DDR地址
    for (i = 0; i < devopt->dev->bufnum; i++) {
        if (devopt->dev->id == 0) {
            pdevData->c2h_fpga_ddr_addr[i] = DEV_DDR_BASE_ADDR + 0x40000000 + (unsigned int)i * 0x01000000;
        } else {
            pdevData->c2h_fpga_ddr_addr[i] = DEV_DDR_BASE_ADDR + (unsigned int)i * 0x01000000;
        }
    }

    
  
    devopt->read_frame = xdmaRead218;

    devopt->dev->devData = pdevData;
    return 0;
    
cleanup:
    // 清理资源（需要实现对应的清理函数）

    if (pdevData) {
        // 关闭文件描述符
        if (pdevData->gpio_fd >= 0) close(pdevData->gpio_fd);
        // 释放内存
        if (pdevData->c2h_fpga_ddr_addr) free(pdevData->c2h_fpga_ddr_addr);
        free(pdevData);
    }
    return -5;
}
/**
 * XDMA设备初始化
 * @param ctx 设备上下文指针（输入输出参数）
 * @return 成功返回0，失败返回负的错误码
 */
int ImgGetXdmaInit(void **ctx)
{
    int i;
    int ret = 0;
    char cmdstr[100] = {0};
    size_t devidlen = 0;
    size_t devnamelen = 0;

    // 参数检查
    if (!ctx) {
        printf("Error: Invalid context pointer\n");
        return -1;
    }

    ImgGetXdmaInitData *p = (ImgGetXdmaInitData *)(*ctx);
    if (!p) {
        printf("Error: Invalid init data\n");
        return -2;
    }

    // 分配设备操作结构体
    ImgXdmaGetOpt *devopt = (ImgXdmaGetOpt*)malloc(sizeof(ImgXdmaGetOpt));
    if (!devopt) {
        printf("Error: Failed to allocate ImgXdmaGetOpt\n");
        return -3;
    }

    // 计算字符串长度
    devidlen = strlen(p->dev_id) + 1;
    devnamelen = strlen(p->dev_name) + 1;

    // 分配设备数据内存
    devopt->dev = (ImgGetXdmaInitData*)malloc(sizeof(ImgGetXdmaInitData) + devidlen + devnamelen);
    if (!devopt->dev) {
        printf("Error: Failed to allocate device data\n");
        goto cleanupXdmaInit;
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

    // 加载内核模块
    snprintf(cmdstr, sizeof(cmdstr), "sudo insmod %s", p->dev_name);
    ret = system(cmdstr);
    if (ret != 0) {
        printf("Warning: Failed to sudo insmod %s (ret=%d)\n", p->dev_name, ret);
        snprintf(cmdstr, sizeof(cmdstr), "insmod %s", p->dev_name);
        ret = system(cmdstr);
        if (ret != 0) {
            printf("Warning: Failed to insmod %s (ret=%d)\n", p->dev_name, ret);
        }
    }

    // 打开XDMA设备文件
    devopt->dev->c2h_dma_fd = open("/dev/xdma0_c2h_0", O_RDWR | O_NONBLOCK);
    if (devopt->dev->c2h_dma_fd < 0) {
        printf("Error: Failed to open /dev/xdma0_c2h_0\n");
        goto cleanupXdmaInit;
    }

    devopt->dev->control_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
    if (devopt->dev->control_fd < 0) {
        printf("Error: Failed to open /dev/xdma0_user\n");
        goto cleanupXdmaInit;
    }

    devopt->dev->events0_fd = open("/dev/xdma0_events_0", O_RDWR | O_SYNC);
    if (devopt->dev->events0_fd == -1) {
        printf("Error: Failed to open /dev/xdma0_events_0\n");
        goto cleanupXdmaInit;
    }
    // 映射控制寄存器
    devopt->dev->control_base = (unsigned char*)mmap_control(devopt->dev->control_fd, 0x40);
    if (!devopt->dev->control_base) {
        printf("Error: Failed to mmap control registers\n");
        goto cleanupXdmaInit;
    }

    // 初始化队列
    if (queue_init(&(devopt->dev->queue), devopt->dev->bufnum) != 0) {
        printf("Error: Failed to initialize queue\n");
        goto cleanupXdmaInit;
    }

    // 分配图像缓冲区
    devopt->dev->img = (unsigned char**)malloc((unsigned int)(devopt->dev->bufnum) * sizeof(char*));
    if (!devopt->dev->img) {
        printf("Error: Failed to allocate image buffer array\n");
        goto cleanupXdmaInit;
    }

    for (i = 0; i < devopt->dev->bufnum; i++) {
        devopt->dev->img[i] = (unsigned char*)malloc(devopt->dev->width * devopt->dev->height * 2);
        if (!devopt->dev->img[i]) {
            printf("Error: Failed to allocate image buffer %d\n", i);
            goto cleanupXdmaInit;
        }
        memset(devopt->dev->img[i], 0, devopt->dev->width * devopt->dev->height * 2);
        devopt->dev->queue.Tdata.buffer[i] = devopt->dev->img[i];
    }
    // xdma218特定初始化
    if (strcmp(devopt->dev->dev_id, "xdma218") == 0) {
        ret = xdmaInit218(devopt);
        if (ret != 0) {
            printf("Failed xdmaInit218\n");
            goto cleanupXdmaInit;
        }
    }else{
        
    }
    // 设置操作函数
    devopt->put_frame = xdmaPut;
    devopt->clean_frame = xdmaClean;
    // 返回设备句柄
    *ctx = (void*)devopt;
    return 0;

cleanupXdmaInit:
    // 清理资源（需要实现对应的清理函数）
    if(devopt){
        if(devopt->dev){
            if (devopt->dev->control_base) unmap_control((void*)(devopt->dev->control_base), 0x40);
            if (devopt->dev->c2h_dma_fd >= 0) close(devopt->dev->c2h_dma_fd);
            if (devopt->dev->control_fd >= 0) close(devopt->dev->control_fd);
            if (devopt->dev->events0_fd >= 0) close(devopt->dev->events0_fd);
            if (devopt->dev->img) {
                for (i = 0; i < devopt->dev->bufnum; i++) {
                    if (devopt->dev->img[i]) free(devopt->dev->img[i]);
                }
                free(devopt->dev->img);
            }
            queue_destroy(&(devopt->dev->queue));
            free(devopt->dev);
        }
        free(devopt);
    }
    return -5;
}
