
#include "img_get.h"

int main(int argc, char** argv) 
{
	int i;
	ImgGet* imgdev;
	ImgGetInit(&imgdev, "xdma218", "/home/xdma.ko", 1080, 1920, 0, 36 , 3);
	//ImgGetInit(&imgdev, "xdma218", "/home/xdma.ko", 1080, 1920, 1, 8 , 3);
	unsigned char *img;
	

	while(1){
		ImgGetRead(imgdev->ctx, &img);
		ImgPut(imgdev->ctx);
	}
	

	FILE* fp;
	fp = fopen("b.raw","w+b");
	fwrite(img,1,1920*1080*2,fp);
	fclose(fp);

    return 0;
}

