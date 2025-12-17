#ifndef _IMGLOCAL_H_
#define _IMGLOCAL_H_
#include <sys/mman.h>       // 为了 mmap, PROT_READ, PROT_WRITE, MAP_SHARED
#include <sys/types.h>      // 为了 open, read, lseek
#include <sys/stat.h>       // 为了 open
#include <fcntl.h>          // 为了 O_RDWR, O_NONBLOCK, O_SYNC, O_RDONLY
#include <unistd.h>         // 为了 read, lseek, close
#include <poll.h>           // 为了 poll, struct pollfd, POLLPRI
#include <stdlib.h>     // 添加这行：为了 malloc, free
#include <stdio.h>      // 可选：为了调试输出
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <string.h>     // 可选：为了 memset
#include "queue.h"

#if defined(__cplusplus)
extern "C" {
#endif

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

#pragma pack()

typedef struct ImgGetLocalInitData_ {
    unsigned int width;		  ///< 宽度
	unsigned int height;		  ///< 高度
    char* dev_id;
    char* dev_name;
    int id;
    int gpio;
    int bufnum;
    void* devData;
    int timer_fd;
    int epoll_fd;
    struct epoll_event ev;
    unsigned char** img;
    TaskQueue queue;
} ImgGetLocalInitData;

typedef struct ImgLocalGetOpt_ {
    int (*read_frame)(void* ctx, unsigned char** img);
    int (*put_frame)(void* ctx);
    int (*clean_frame)(void* ctx);
    ImgGetLocalInitData *dev;
} ImgLocalGetOpt;

int ImgGetLocalInit(void **ctx);

#if defined(__cplusplus)
}
#endif
#endif
