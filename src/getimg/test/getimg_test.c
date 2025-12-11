
#include "img_get.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
void* readThread0(void* arg)
{
	unsigned char *img;
	ImgGet* imgdev;
	ImgGetInit(&imgdev, "xdma218", "/home/xdma.ko", 1080, 1920, 0, 36 , 3);
	
	int i = 0;
	FILE* fp;
	char cmdstr[100] = {0};
	int ret;
	while(1){
		ret = ImgGetRead(imgdev->ctx, &img);
		memset(cmdstr, 0, 100);
		sprintf(cmdstr,"a%d.raw",i);
		fp = fopen(cmdstr,"w+b");
		fwrite(img,1,1920*1080*2,fp);
		fclose(fp);
		printf("readThread0: ret = %d\n", ret);
		ImgPut(imgdev->ctx);
		i++;
	}

    return NULL;
}
void* readThread1(void* arg)
{
	unsigned char *img;
	ImgGet* imgdev;
	ImgGetInit(&imgdev, "xdma218", "/home/xdma.ko", 1080, 1920, 1, 8 , 3);
	int i = 0;
	FILE* fp;
	char cmdstr[100] = {0};
	int ret;
	while(1){
		ret = ImgGetRead(imgdev->ctx, &img);
		memset(cmdstr, 0, 100);
		sprintf(cmdstr,"b%d.raw",i);
		fp = fopen(cmdstr,"w+b");
		fwrite(img,1,1920*1080*2,fp);
		fclose(fp);
		printf("readThread1: ret = %d\n", ret);
		ImgPut(imgdev->ctx);
		i++;
	}
	
    return NULL;
}
int main(int argc, char** argv) 
{
	pthread_t threadRead;
	pthread_t threadRead1;
	pthread_create(&threadRead, NULL, readThread0, NULL);
	pthread_create(&threadRead1, NULL, readThread1, NULL);

    pthread_join(threadRead1, NULL);	
    pthread_join(threadRead, NULL);
    return 0;
}

