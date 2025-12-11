#include "img_get.h"
#include "img_xdma.h"

// 棋盘格数据
static unsigned char* bufimg;
static int fillChessboard(unsigned char* buf, int h, int w, int blockSize)
{
	int y,x;
	int offset;
	bool isWhite;
	unsigned short value;
	for(y=0;y<h;y++){
		for(x=0;x<w;x++){
			offset = (y * w + x) * 2;
			isWhite = ((x/blockSize)+(y/blockSize)) % 2 == 0;
			value = isWhite ? 50000 : 1000;
			buf[offset] = value & 0xff;
			buf[offset + 1] = (value >> 8) & 0xff;
			//printf("··%d\n",value);
		}
	}
    return 0;
}
/**
 * 初始化图像采集设备
 * @param imgdev 输出参数，返回初始化后的设备句柄
 * @param devid 设备ID标识（如"mipi"、"xdma"等）
 * @param devname 设备名称
 * @param h 图像高度
 * @param w 图像宽度
 * @param id 设备编号
 * @param gpio GPIO引脚号
 * @param bufnum 缓冲区数量
 * @return 成功返回0，失败返回-1
 */
int ImgGetInit(ImgGet** imgdev, const char* devid, const char* devname, int h, int w, int id, int gpio, int bufnum)
{
    int ret = 0;
    char cmdstr[100] = {0};
    int devidlen = 0;
    int devnamelen = 0;
    
    // 分配ImgGet主结构体
    ImgGet* imgp = (ImgGet*)malloc(sizeof(ImgGet));
    if (!imgp) {
        printf("Error: Failed to allocate ImgGet structure\n");
        return -1;
    }
    
    // 计算字符串长度（包含结束符）
    devidlen = strlen(devid) + 1;
    devnamelen = strlen(devname) + 1;

    // 分配设备初始化数据结构体及字符串空间
    imgp->dev = (ImgGetInitData*)malloc(sizeof(ImgGetInitData) + devidlen + devnamelen);
    if (!imgp->dev) {
        printf("Error: Failed to allocate ImgGetInitData structure\n");
        free(imgp);
        return -1;
    }
    
    // 初始化设备参数
    imgp->dev->width = w;
    imgp->dev->height = h;
    imgp->dev->dev_id = (char *)(imgp->dev + 1);           // 字符串紧随结构体之后
    imgp->dev->dev_name = (char *)(imgp->dev + 1 + devidlen); // 第二个字符串
    imgp->dev->id = id;
    imgp->dev->gpio = gpio;
    imgp->dev->bufnum = bufnum;
    
    // 复制设备ID和名称字符串
    memcpy(imgp->dev->dev_id, devid, devidlen);
    memcpy(imgp->dev->dev_name, devname, devnamelen);
    
    imgp->ctx = (void*)imgp->dev;  // 上下文指向设备数据

    // 根据设备类型执行相应初始化
    if (strcmp(devid, "mipi") == 0 || strcmp(devid, "MIPI") == 0) {
        // MIPI设备初始化（暂未实现）
        // ret = mipi_camera_init(&imgp->ctx);
    } 
    else if (strncmp(devid, "xdma", 4) == 0 || strncmp(devid, "XDMA", 4) == 0) {
        // XDMA设备初始化
        ret = ImgGetXdmaInit(&imgp->ctx);
        if (ret != 0) {
            printf("Error: XDMA device initialization failed\n");
            free(imgp->dev);
            free(imgp);
            return -1;
        }
    }
    else {
        // 未知设备类型
        printf("Error: Unknown device type: %s\n", devid);
        free(imgp->dev);
        free(imgp);
        return -1;
    }

    // 棋盘格数据初始化
    bufimg = (unsigned char*)malloc(sizeof(unsigned char) * (w*h*2));
    if (!bufimg) {
        printf("Error: Failed to allocate bufimg structure\n");
        free(imgp->dev);
        free(imgp);
        return -1;
    }
    fillChessboard(bufimg, h, w, 32);

    *imgdev = imgp;  // 返回设备句柄
    return 0;
}
/**
 * 输出图像帧（调用底层驱动接口）
 * @param ctx 设备上下文指针
 * @return 总是返回0
 */
int ImgPut(void *ctx)
{
    if (!ctx) {
        return -1; // 增加空指针检查
    }
    
    ImgGetOpt *p = (ImgGetOpt* )ctx;
    
    // 检查函数指针有效性
    if (p && p->put_frame) {
        p->put_frame(ctx);
    }
    
    return 0;
}

/**
 * 读取图像帧数据
 * @param ctx 设备上下文指针
 * @param img 输出参数，返回图像数据指针
 * @return 成功返回0，失败返回-1
 */
int ImgGetRead(void *ctx, unsigned char** img)
{
	int ret = 0;
    // 参数有效性检查
    if (!ctx || !img) {
        printf("Error: Invalid parameters\n");
        return -1;
    }
    
    ImgGetOpt *p = (ImgGetOpt* )ctx;
    
    // 检查函数指针有效性
    if (!p || !p->read_frame) {
        printf("Error: Invalid function pointer\n");
        return -1;
    }

    ret = p->read_frame(ctx, img);
    if(ret == -1){ //超时没返回,输出棋盘格
        *img = bufimg;
    }
    
    return 0;
}

int ImgGetClean(ImgGet** imgdev)
{
	ImgGet* imgp = *imgdev;
	ImgGetOpt *p = (ImgGetOpt* )imgp->ctx;
	p->clean_frame(imgp->ctx);
	free(imgp->dev);
    free(imgp);
    free(bufimg);
	return 0;
}
