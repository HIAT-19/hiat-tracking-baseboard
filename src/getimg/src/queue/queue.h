#ifndef _QUEUE_H_
#define _QUEUE_H_
#include <pthread.h>    // 为了 pthread_mutex_t, pthread_cond_t
#include <stdbool.h>    // 为了 bool, true, false
#include <stddef.h>     // 为了 NULL
#include <stdlib.h>     // 添加这行：为了 malloc, free
#include <stdio.h>      // 可选：为了调试输出
#include <string.h>     // 可选：为了 memset
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

#define MAX_TASKS 512

#define FLAG_1 (0x01)
#define FLAG_2 (0x02)
#define FLAG_3 (0x04)
#define FLAG_4 (0x08)
#define FLAG_5 (0x10)
#define FLAG_6 (0x20)
#define FLAG_7 (0x40)
#define FLAG_8 (0x80)

typedef struct {
    int free_buf_id;
    UINT64 time;
    void *usrdata;
    int flag;
} Task;
typedef struct {
    unsigned char **buffer;
    unsigned char **buffer8;
} Taskdata;
typedef struct {
   // Task tasks[MAX_TASKS];
    Task **tasks;
    Taskdata Tdata;
    int front;  
    int rear;
    int count;
    int num;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TaskQueue;

#pragma pack(1)

#pragma pack()

int queue_init(TaskQueue *queue, int num);
int queue_destroy(TaskQueue *queue);
int queue_push(TaskQueue *queue, Task task, int flag);
Task queue_pop(TaskQueue *queue, int flag);
int queue_kong(TaskQueue *queue);
Task queue_pop_main(TaskQueue *queue, int flag);
Task queue_acquire_or_modify_task(TaskQueue *queue, Task taskin, int flag, int getflag);
#if defined(__cplusplus)
}
#endif
#endif
