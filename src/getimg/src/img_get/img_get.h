#ifndef _IMGGET_H_
#define _IMGGET_H_

#include <stdlib.h>    // 添加：为了 malloc, free
#include <string.h>    // 添加：为了 strlen, strcmp, strncmp, memcpy
#include <stdio.h>     // 添加：为了 printf
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif
#define GET_IMG_UYVY  (1)
#pragma pack(1)

#pragma pack()

typedef struct ImgGetInitData_ {
    unsigned int width;		  ///< 宽度
	unsigned int height;		  ///< 高度
    char* dev_id;           //设备类型，xdma，xdma218，mipi, local
    char* dev_name;         //设备名，/dev/video0，/home/xdma.ko, /userdata/img/img_%d.raw
    int id;                 //通道（xdma：0或1），mipi:V4L2_PIX_FMT_UYVY，local:从哪里开始数字
    int gpio;               //gpio（xdma218：中断io），local:定时器间隔时间例如20
    int bufnum;             //缓存数，
} ImgGetInitData;

typedef struct ImgGetOpt_ {
    int (*read_frame)(void* ctx, unsigned char** img);
    int (*put_frame)(void* ctx);
    int (*clean_frame)(void* ctx);
} ImgGetOpt;

typedef struct ImgGet_ {   
    unsigned char   *img;
    void *ctx;
    ImgGetInitData *dev;
} ImgGet;

// 结构体二级指针，设备类型（xdma，xdma218，mipi），设备名（/dev/video0，/home/xdma.ko）,分辨率h，分辨率w，通道（xdma：0或1），gpio（xdma218：中断io），缓存数3
int ImgGetInit(ImgGet** imgdev, const char* devid, const char* devname, int h, int w, int id, int gpio, int bufnum);
// ctx：imgdev下的ctx，img：填充图像数据到*img
int ImgGetRead(void *ctx, unsigned char** img);
int ImgPut(void *ctx);
int ImgGetClean(ImgGet** imgdev);
void TransferToBYTEIMG1(ImgGet** imgdev, uint8_t* Image16, uint8_t* IMG2, uint32_t *pHist16To8, uint8_t *pLUT16To8 ); //TransferToBYTEIMG1(in, ou,  g_pHist16To8, g_pLUT16To8 );
#if defined(__cplusplus)
}
#endif
#endif
