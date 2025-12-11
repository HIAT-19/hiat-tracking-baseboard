#include "queue.h"

// 初始化队列
int queue_init(TaskQueue *queue, int num) {
    int i;
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    queue->Tdata.buffer = (unsigned char**)malloc((unsigned int)num * sizeof(char*));
    queue->tasks = (Task**)malloc((unsigned int)num * sizeof(Task*));
    for(i = 0; i < num; i++){
        queue->tasks[i] = (Task*)malloc(sizeof(Task));
        memset(queue->tasks[i], 0, sizeof(Task));
    }
    queue->num = num;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return 0;
}

// 销毁队列
int queue_destroy(TaskQueue *queue) {
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    return 0;
}

// 入队操作
// 返回值：有空间返回非-数，-1成功写入，-2没有空间
int queue_push(TaskQueue *queue, Task task, int flag) {
    pthread_mutex_lock(&queue->mutex);
    //printf("ddddd\n");
    if (flag) {
        while (queue->count == queue->num) {
            //pthread_cond_wait(&queue->cond, &queue->mutex);
            pthread_mutex_unlock(&queue->mutex);
            return -2;
        }
        //intf("ndsdasssd\n");
        pthread_mutex_unlock(&queue->mutex);
        return queue->rear;
    }else {
        *queue->tasks[queue->rear] = task;
        queue->rear = (queue->rear + 1) % queue->num;
        queue->count++;
        // p->flag = true;
        pthread_cond_signal(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
}

// 出队操作
Task queue_pop(TaskQueue *queue, int flag) {
    Task task = {-1};
    pthread_mutex_lock(&queue->mutex);

    if (flag) {
        while (queue->count == 0) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        task = *queue->tasks[queue->front];
    }else {
        queue->front = (queue->front + 1) % queue->num;
        queue->count--;
        // p->flag = false;
    }
    //pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return task;
}
// 判断是否空
int queue_kong(TaskQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    if(queue->count == 0){
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }
    pthread_mutex_unlock(&queue->mutex);
    return 1;
}