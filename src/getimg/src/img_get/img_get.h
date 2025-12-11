#ifndef _IMGGET_H_
#define _IMGGET_H_

#include <stdlib.h>    // 添加：为了 malloc, free
#include <string.h>    // 添加：为了 strlen, strcmp, strncmp, memcpy
#include <stdio.h>     // 添加：为了 printf
#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(1)

#pragma pack()

typedef struct ImgGetInitData_ {
    unsigned int width;		  ///< 宽度
	unsigned int height;		  ///< 高度
    char* dev_id;
    char* dev_name;
    int id;
    int gpio;
    int bufnum;
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
int ImgGetRead(void *ctx, unsigned char** img);
int ImgPut(void *ctx);
int ImgGetClean(ImgGet** imgdev);

#if defined(__cplusplus)
}
#endif
#endif
