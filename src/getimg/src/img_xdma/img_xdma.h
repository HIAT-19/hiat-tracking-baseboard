#ifndef _IMGXDMA_H_
#define _IMGXDMA_H_
#include <sys/mman.h>       // 为了 mmap, PROT_READ, PROT_WRITE, MAP_SHARED
#include <sys/types.h>      // 为了 open, read, lseek
#include <sys/stat.h>       // 为了 open
#include <fcntl.h>          // 为了 O_RDWR, O_NONBLOCK, O_SYNC, O_RDONLY
#include <unistd.h>         // 为了 read, lseek, close
#include <poll.h>           // 为了 poll, struct pollfd, POLLPRI
#include <stdlib.h>     // 添加这行：为了 malloc, free
#include <stdio.h>      // 可选：为了调试输出
#include <string.h>     // 可选：为了 memset
#include "queue.h"

#if defined(__cplusplus)
extern "C" {
#endif
#define MAP_SIZE (2*16UL)
#define DEV_DDR_BASE_ADDR (0x0U)
#define VIDEO_FRAME_STORE_NUM 6
typedef unsigned char UINT08;
typedef unsigned short  UINT16;
typedef unsigned int  UINT32;
typedef unsigned long long UINT64;
typedef char INT08;
typedef short  INT16;
typedef int  INT32;
typedef   signed char   int8_t;
typedef unsigned char  uint8_t;
typedef          short  int16_t;
typedef unsigned short uint16_t;
typedef          int    int32_t;
typedef unsigned int   uint32_t;
#pragma pack(1)
typedef struct
{
    unsigned short camera_id;
    unsigned short buffer_index;
    unsigned short in_speed;
    unsigned short out_speed;
    unsigned int buffer_count;
    unsigned int ddr_error_count;
}Fpgacamera218, * PFpgacamera218;
typedef struct
{
    UINT32 camera_id;
    UINT32 speed;
    UINT32 count;
    UINT32 addr;
    UINT32 speed1;
    UINT32 count1;
    UINT32 addr1;
}Fpgacamerayk, * PFpgacamerayk;
#pragma pack()
typedef struct ImgXdmaInitDatayk_ {
    int c2h_dma_fd;
	int control_fd;
	int events0_fd;
    int gpio_fd;
    unsigned int *c2h_fpga_ddr_addr;
    struct pollfd fds[1];
    Fpgacamerayk fpgacamera;
    unsigned char** img;
    TaskQueue queue;
    unsigned char* control_base;
} ImgXdmaInitDatayk;
typedef struct ImgXdmaInitData218_ {
    int c2h_dma_fd;
	int control_fd;
	int events0_fd;
    int gpio_fd;
    unsigned int *c2h_fpga_ddr_addr;
    struct pollfd fds[1];
    Fpgacamera218 fpgacamera;
    unsigned char** img;
    TaskQueue queue;
    unsigned char* control_base;
} ImgXdmaInitData218;
typedef struct ImgGetXdmaInitData_ {
    unsigned int width;		  ///< 宽度
	unsigned int height;		  ///< 高度
    char* dev_id;
    char* dev_name;
    int id;
    int gpio;
    int bufnum;
    void* devData;
} ImgGetXdmaInitData;

typedef struct ImgXdmaGetOpt_ {
    int (*read_frame)(void* ctx, unsigned char** img);
    int (*put_frame)(void* ctx);
    int (*clean_frame)(void* ctx);
    ImgGetXdmaInitData *dev;
} ImgXdmaGetOpt;

int ImgGetXdmaInit(void **ctx);

#if defined(__cplusplus)
}
#endif
#endif
