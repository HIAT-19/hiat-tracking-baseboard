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

typedef struct {
    int free_buf_id;
} Task;
typedef struct {
    unsigned char **buffer;
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

#if defined(__cplusplus)
}
#endif
#endif
