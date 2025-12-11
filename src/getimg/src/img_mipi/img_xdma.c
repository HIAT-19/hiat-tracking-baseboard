#include "img_get.h"


static void* mmap_control(int fd, long mapsize)
{
    void* vir_addr;
    vir_addr = mmap(0, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return vir_addr;
}

int ImgXdmaInit(unsigned char* devname)
{
    int ret = 0;
	char cmdstr[100]={0};
    //加载驱动

	sprintf(cmdstr,"sudo insmod %s",devname);
    ret = system(cmdstr);
	if(ret |= 0){
		printf("system insmod xdma.ko err\n");
	}

 	c2h0_dma_fd = open("/dev/xdma0_c2h_0", O_RDWR | O_NONBLOCK);
    if ((a->imgData->c2h_dma_fd) < 0) {
		printf("Failed to open xdma0_c2h_0 device\n");
        return -1;
    }

    control_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
    if ((a->imgData->control_fd) < 0) {
		printf("Failed to open xdma0_user device\n");
       return -2;
    }
	events0_fd=open("/dev/xdma0_events_0",O_RDWR|O_SYNC );
	if(a->imgData->events0_fd==-1){
		printf("open event error\n");
		return -3;
	} 
	
    a->imgData->control_base = (UINT08*)mmap_control((a->imgData->control_fd), 0x40);

	
    return 0;
}
void fillChessboard(UINT08* buf)
{
	int y,x;
	int offset;
	int blockSize = 32;
	bool isWhite;
	unsigned short value;
	for(y=0;y<512;y++){
		for(x=0;x<640;x++){
			offset = (y * 640 + x) * 2;
			isWhite = ((x/blockSize)+(y/blockSize)) % 2 == 0;
			value = isWhite ? 50000 : 1000;
			buf[offset] = value & 0xff;
			buf[offset + 1] = (value >> 8) & 0xff;
			//printf("··%d\n",value);
		}
	}
}
#if 0
int ImgRead(Param* a,UINT08 *control_base, int c2h_dma_fd, int fd, UINT08 *buf)
{
    int val = 0,ret = -1;
    Fpgacamera *data;
	UINT32 addr;
    read(fd, &val,4); 
    if(val){
        data = (Fpgacamera* )(control_base);
        XW_LOG_DEBUG("camera_id = %d\n", data->camera_id);

        XW_LOG_DEBUG("speed = %d\n", data->speed);
        XW_LOG_DEBUG("count = %d\n", data->count);
        //XW_LOG_DEBUG("addr = 0x%08X\n", data->addr);
        
        // XW_LOG_DEBUG("speed1 = %d\n", data->speed1);
        // XW_LOG_DEBUG("count1 = %d\n", data->count1);
        // XW_LOG_DEBUG("addr1 = 0x%08X\n", data->addr1);

        lseek(c2h_dma_fd, data->addr, SEEK_SET);

        read(c2h_dma_fd, buf, ImgSize_channle_0);
        ret = 0;
    }

    return ret;
}
#else

// #include <linux/videodev2.h>
// #include <sys/ioctl.h>
// int test()
// {
// 	int fd = open("/dev/video0", O_RDWR);
// 	if(fd < 0)
// 	{
// 		printf("open video faield!\n");
// 		return -1;
// 	}
// 	struct v4l2_format fmt;
// 	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
// 	fmt.fmt
// }
 
int ImgRead(Param* a,UINT08 *control_base, int c2h_dma_fd, int fd, UINT08 *buf)
{
	int ret = -1;

    static HDImage *hd_image = new HDImage{};

	// a->modeAlg = 0x01;
	// a->myMisc->Data.distance_X = a->myMisc->gdDataAlg->nue[0] + 6;
    // a->myMisc->Data.distance_Y = 1000;
    // a->myMisc->Data.distance_Z = a->myMisc->gdDataAlg->nue[2] + 38;
    // a->myMisc->Data.tar_FY = a->sysState->GdData.fy;
    // a->myMisc->Data.tar_PH = a->sysState->GdData.ph;//
    // a->myMisc->Data.tar_GZ = a->sysState->GdData.hg;
	// a->myMisc->Data.FrameFY = a->sysState->Data.FrameFY;
	// a->myMisc->Data.FramePH = a->sysState->Data.FramePH;

    for(int i = 0; ; ++i) {
		hd_get_mipi_video_frame(mipi_dev, hd_image);
		if (hd_image->data != NULL) {
		//fprintf(stdout, "Get mipi frame success, image width: %ld, image height: %ld\n", hd_image->param->width, hd_image->param->height);
			
			// FILE *fp = fopen("mipi.raw", "wb");
			// fwrite(hd_image->data, 1, 640 * 512 * 2, fp);
			// fclose(fp);
			memcpy(buf,hd_image->data,640*512*2);
			hd_free_mipi_frame(mipi_dev, (HDMipiBuffer *)hd_image->hd_buf);
			ret = 0;

			break;
		}
		else {
			//fprintf(stdout, "Get mipi frame NULL\n");
			//std::this_thread::sleep_for(std::chrono::milliseconds(5));
			usleep(100);
		}
    }

    return ret;
}
// int ImgRead(Param* a,UINT08 *control_base, int c2h_dma_fd, int fd, UINT08 *buf)
// {
//     int val = 0,ret = -1;
//     Fpgacamera *data;
// 	UINT32 addr = 0xffffffff;
//     static int fl = 0;
// 	static int flga = 0;
// 	static UINT08 buff[640*512*2];
// 	if(flga == 0){
// 		fillChessboard(buff);
// 		flga = 1;
// 	}
	
// 	data = (Fpgacamera* )(control_base);
// 	while(1){
// 		if(a->imgChannelSelect == 0){
// 			if(data->count > 1){
// 				printf("count = %d\n", data->count);
// 				printf("addr = 0x%08X\n", data->addr);
// 			}else if(data->count == 1){
// 				addr = data->addr;
// 				fl = 0;
// 				//printf("qqqqqqqqq\n");
// 				break;
// 			}else{
// 				usleep(100);
// 				fl++;
// 				if(fl == 221){
// 					break;
// 				}
// 				//printf("1qqqqqqqqq\n");
// 			}	
// 		}else{
// 			if(data->count1 > 1){
// 				printf("count1 = %d\n", data->count1);
// 				printf("addr1 = 0x%08X\n", data->addr1);
// 			}else if(data->count1 == 1){
// 				addr = data->addr1;
// 				break;
// 			}else{
// 				usleep(100);
// 			}	
// 		}
// 	}
// 	if(addr != 0xffffffff){
// 		lseek(c2h_dma_fd, addr, SEEK_SET);
//         read(c2h_dma_fd, buf, ImgSize_channle_0);
// 		ret = 0;
// 	}
// 	if(fl == 221){
// 		memcpy(buf,buff,640*512*2);
// 		fl = 0;
// 		ret = 1;
// 		//printf("2qqqqqqqqq\n");
// 	}
//     return ret;
// }
/*int ImgRead(Param* a,UINT08 *control_base, int c2h_dma_fd, int fd, UINT08 *buf)
{
    int val = 0,ret = 0;
    Fpgacamera *data;
	UINT32 addr = 0xffffffff;
    static int fl = 0;
	static int flga = 0;
	static UINT08 buff[ImgSize_channle_0+1024];
	FILE* fp = NULL;
	static int i = 96007;//32300;//38500,46700,50500   //27380 37302
	char cmdstr[100]={0};
	ZTZH_GD ztgd;
	static int flagct = 1;

	//惯导对其，初值,24800
	if(flagct){
		a->modeAlg = 0x01;//zi,guaf,tiaoshibb
		fp = fopen("/userdata/img11/img_46745.dat","r+b"); //24800 3607
		fread(buff, 1, ImgSize_channle_0+1024,fp);
		fclose(fp);
		fp = NULL;
		memcpy(&(a->sysState->GdData), buff+ImgSize_channle_0+sizeof(G_DataFrameIntegrate), sizeof(G_GD_DATA));//惯导	

		ztgd.ph = a->sysState->GdData.ph;
		ztgd.fy = a->sysState->GdData.fy;
		ztgd.hg = a->sysState->GdData.hg;
		ztgd.weidu = a->sysState->GdData.weidu;
		ztgd.jindu = a->sysState->GdData.jindu;
		ztgd.gaodu = a->sysState->GdData.gaodu;
		gd_mubiaojilu(a,&ztgd);
		flagct =0;
	}


	
	while(fp == NULL){
		sprintf(cmdstr,"/userdata/img11/img_%d.dat",i);
		fp = fopen(cmdstr,"r+b");
		// i++;
		i++;
	}
	fread(buff, 1, ImgSize_channle_0+1024,fp);
	memcpy(buf,buff,ImgSize_channle_0);
	fclose(fp);
	fp = NULL;
	//i++;

	// if(i==47200){
	// 	i = 50500;
	// }



	memcpy(&(a->sysState->GdData), buff+ImgSize_channle_0+sizeof(G_DataFrameIntegrate), sizeof(G_GD_DATA));//惯导	
	memcpy(&(a->sysState->Data), buff+ImgSize_channle_0, sizeof(G_DataFrameIntegrate));
	printf("i = %d, xyz = %f %f %f\n",i,a->sysState->GdData.ph,a->sysState->GdData.fy,a->sysState->GdData.hg);
	gd_realtodouble(a);
	lla_to_nue(a->myMisc->gdDataAlg->llaReal, a->myMisc->gdDataAlg->llaTar, a->myMisc->gdDataAlg->nue);
	pthread_mutex_lock(&a->mutexXYZ);
	a->myMisc->Data.distance_X = a->myMisc->gdDataAlg->nue[0] + 6;
    a->myMisc->Data.distance_Y = a->myMisc->gdDataAlg->nue[1] + 24;
    a->myMisc->Data.distance_Z = a->myMisc->gdDataAlg->nue[2] + 38;
    a->myMisc->Data.tar_FY = a->sysState->GdData.fy;
    a->myMisc->Data.tar_PH = a->sysState->GdData.ph;//
    a->myMisc->Data.tar_GZ = a->sysState->GdData.hg;
	a->myMisc->Data.FrameFY = a->sysState->Data.FrameFY;
	a->myMisc->Data.FramePH = a->sysState->Data.FramePH;
	pthread_mutex_unlock(&a->mutexXYZ);
	
	usleep(19000);





    return ret;
}*/
#endif



int ImgGetDe(Param* a)
{
    close(a->imgData->c2h_dma_fd);
    close(a->imgData->control_fd);
    close(a->imgData->events0_fd);
    return 0;
}

#define REAL_IMG_SIZE (640*512)
uint32_t g_pHist16To8[16384];
uint8_t g_pLUT16To8[16384];




void TransferToBYTEIMG1(uint8_t* Image16, uint8_t* IMG2, uint32_t *pHist16To8, uint8_t *pLUT16To8 )
{
 	uint8_t* DATA= (uint8_t*)Image16;
 	int i, imagesize = REAL_IMG_SIZE, Sum, Threshold = 0.001 * REAL_IMG_SIZE, Scale = 0;
 	uint16_t StartV = 0, EndV = 0, *tmpPoint = NULL;
 	uint32_t *hist  = (uint32_t *)(pHist16To8);
 	uint8_t  *LUT   = (uint8_t *)(pLUT16To8);
 	uint8_t *lpImgNow, *lpNow, *lpEnd = DATA + imagesize * 2;
 	memset(hist, 0, 16384 * sizeof(int));
 	for(lpNow = DATA; lpNow < lpEnd; lpNow += 2)
 	{
 		tmpPoint = (uint16_t *)(lpNow);
 		hist[(*tmpPoint)>>2]++;  
 	}
 	Sum = 0;
 
 	for (i = 2; i <= 16381; i++)
 	{
 		Sum += hist[i];
 		if (Sum > Threshold)
 		{
 			StartV = i;
 			break;
 		}
 	}
 	Sum = 0;
 	for (i = 16381; i >= 2; i--)
 	{
 		Sum += hist[i];
 		if (Sum > Threshold)
 		{
 			EndV = i;
 			break;
 		}
 	}
 
 	if( abs(EndV - StartV) > 1 )
 	{
 		Scale = (int)(255 * 65536 / (EndV - StartV));//点源目标可能为0，会引起除法部件异常
 	}
 	else
 	{
 		Scale = (int)(255 * 65536);
 	}
 
 	for (i = 0; i < StartV; i++)
 		LUT[i] = 0;
 	for (i = EndV; i < 16384; i++)
 		LUT[i] = 255;
 	for (i = StartV; i < EndV; i++)
 		LUT[i] = (uint8_t)(((i - StartV) * Scale) >> 16);
 	for(lpNow = DATA, lpImgNow = IMG2; lpNow < lpEnd; lpNow += 2, lpImgNow++)
 	{	
 		*lpImgNow = LUT[(*(uint16_t *)(lpNow))>>2];
 	}

	return;
}


int Img16to8(Param* a)
{
    UINT08 o;
    UINT08 c;
    UINT16 *d;
    UINT08 *in,*ou;
    in = a->imgData->buf;
    ou = a->imgData->buf8;


    TransferToBYTEIMG1(in, ou,  g_pHist16To8, g_pLUT16To8 );

	//addTestToY400Buffer(ou,640,512,a);

//     for (int i = 0;i < 640 * 512; i++) {
//         // o = in[i*2];
//         // in[i*2] = in[i*2+1];
//         // in[i*2+1] = o;
//  //       d = (UINT16*)(in+i*2);
//  //       d = in[i*2]*256 + in[i*2+1];
//  //       ou[i] = ((in[i*2]*256 + in[i*2+1])* 256.0 / 65536.0);
//   ou[i] = (in[i*2+1]);
//     }
    return 0;
}


int addTestToY400Buffer(UINT08* buffer, int w, int h, Param* a, float *rect)
{
	cv::Mat grayImage(h,w,CV_8UC1,buffer);
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = 0.7;
	int thickness = 2;
	int doutoint;
	
	//id
	std::string text = "ID "+std::to_string(a->sysState->Data.u8FrameCnt);
	//std::string text = "ID:"+std::to_string(a->sysState->Data.u32RealDistance);
	cv::Point position(420,500);
	cv::putText(grayImage,text,position,fontFace,fontScale,cv::Scalar(255),thickness);

	//距离
	text = "Dis "+std::to_string(a->sysState->Data.yc_distance);
	cv::Point position1(420,480);
	cv::putText(grayImage,text,position1,fontFace,fontScale,cv::Scalar(255),thickness);

	//工作状态
	text = "STAT "+std::to_string(a->sysState->u8Status);
	cv::Point position2(0,460);
	cv::putText(grayImage,text,position2,fontFace,fontScale,cv::Scalar(255),thickness);

	//框架角fy
	text = "FY "+std::to_string(a->sysState->Data.FrameFY);
	cv::Point position3(0,480);
	cv::putText(grayImage,text,position3,fontFace,fontScale,cv::Scalar(255),thickness);
	//框架角ph
	text = "PH "+std::to_string(a->sysState->Data.FramePH);
	cv::Point position4(0,500);
	cv::putText(grayImage,text,position4,fontFace,fontScale,cv::Scalar(255),thickness);

	//
#if SJ_GD
	doutoint = static_cast<int>(a->sysState->Data.distance_X);
	text = "X "+std::to_string(doutoint);
	cv::Point position5(0,16);
	cv::putText(grayImage,text,position5,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->sysState->Data.distance_Y);
	//fis = std::to_string(doutoint);
	text = "Y "+std::to_string(doutoint);
	cv::Point position6(0,36);
	cv::putText(grayImage,text,position6,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->sysState->Data.distance_Z);
	//fis = std::to_string(doutoint);
	text = "Z "+std::to_string(doutoint);
	cv::Point position7(0,56);
	cv::putText(grayImage,text,position7,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->sysState->Data.tar_PH);
	text = "PH "+std::to_string(doutoint);
	cv::Point position8(110,16);
	cv::putText(grayImage,text,position8,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->sysState->Data.tar_FY);
	text = "FY "+std::to_string(doutoint);
	cv::Point position9(110,36);
	cv::putText(grayImage,text,position9,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->sysState->Data.tar_GZ);
	text = "GZ "+std::to_string(doutoint);
	cv::Point position10(110,56);
	cv::putText(grayImage,text,position10,fontFace,fontScale,cv::Scalar(255),thickness);
#else
	doutoint = static_cast<int>(a->myMisc->Data.distance_X);
	text = "X "+std::to_string(doutoint);
	cv::Point position5(0,16);
	cv::putText(grayImage,text,position5,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->myMisc->Data.distance_Y);
	//fis = std::to_string(doutoint);
	text = "Y "+std::to_string(doutoint);
	cv::Point position6(0,36);
	cv::putText(grayImage,text,position6,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->myMisc->Data.distance_Z);
	//fis = std::to_string(doutoint);
	text = "Z "+std::to_string(doutoint);
	cv::Point position7(0,56);
	cv::putText(grayImage,text,position7,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->myMisc->Data.tar_PH);
	text = "PH "+std::to_string(doutoint);
	cv::Point position8(110,16);
	cv::putText(grayImage,text,position8,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->myMisc->Data.tar_FY);
	text = "FY "+std::to_string(doutoint);
	cv::Point position9(110,36);
	cv::putText(grayImage,text,position9,fontFace,fontScale,cv::Scalar(255),thickness);
	doutoint = static_cast<int>(a->myMisc->Data.tar_GZ);
	text = "GZ "+std::to_string(doutoint);
	cv::Point position10(110,56);
	cv::putText(grayImage,text,position10,fontFace,fontScale,cv::Scalar(255),thickness);
#endif






	text = "FZID "+std::to_string(a->sysState->Data.u32RealDistance);
	cv::Point position11(0,76);
	cv::putText(grayImage,text,position11,fontFace,fontScale,cv::Scalar(255),thickness);



	text = "X "+std::to_string(320-a->myMisc->u16XsdeviationFW);
	cv::Point position121(0,96);

	// cv::Rect textRect = cv::getTextSize(text,fontFace,fontScale,thickness,nullptr);
	// cv::Mat roi = grayImage(cv::Rect(position121.x,position121.y - textRect.height,textRect.width,textRect.height));
	// cv::Scalar meanColor = cv::mean(roi);
	// double brightness = (meanColor[0] + meanColor[1] + meanColor[2]) / 3.0;
	// cv::Scalar textColor;
	// if(brightness > 127){
	// 	textColor = cv::Scalar(0);
	// }else{
	// 	textColor = cv::Scalar(255);
	// }
	// cv::putText(grayImage,text,position121,fontFace,fontScale,textColor,thickness);

	cv::putText(grayImage,text,position121,fontFace,fontScale,cv::Scalar(255),thickness);


	text = "Y "+std::to_string(256-a->myMisc->u16XsdeviationFY);
	cv::Point position131(110,96);
	cv::putText(grayImage,text,position131,fontFace,fontScale,cv::Scalar(255),thickness);

	
	////
	// std::string fis;
	// doutoint = static_cast<int>(a->sysState->GdData.jindu);
	// fis = std::to_string(doutoint);
	// text = "X "+fis.substr(0,std::min(5,(int)fis.length()));
	// cv::Point position51(420,16);
	// cv::putText(grayImage,text,position51,fontFace,fontScale,cv::Scalar(255),thickness);
	// doutoint = static_cast<int>(a->sysState->GdData.gaodu);
	// fis = std::to_string(doutoint);
	// text = "Y "+fis.substr(0,std::min(5,(int)fis.length()));
	// cv::Point position61(420,36);
	// cv::putText(grayImage,text,position61,fontFace,fontScale,cv::Scalar(255),thickness);
	// doutoint = static_cast<int>(a->sysState->GdData.weidu);
	// fis = std::to_string(doutoint);
	// text = "Z "+fis.substr(0,std::min(5,(int)fis.length()));
	// cv::Point position71(420,56);
	// cv::putText(grayImage,text,position71,fontFace,fontScale,cv::Scalar(255),thickness);

	//doutoint = static_cast<int>(a->myMisc->fpgaData->time_s);
	text = "T "+std::to_string(a->myMisc->fpgaData->time_s);
	cv::Point position51(420,16);
	cv::putText(grayImage,text,position51,fontFace,fontScale,cv::Scalar(255),thickness);


	// doutoint = static_cast<int>(a->myMisc->gdDataAlg->llaReal[1]);
	// text = "Y "+std::to_string(doutoint);
	// cv::Point position61(420,36);
	// cv::putText(grayImage,text,position61,fontFace,fontScale,cv::Scalar(255),thickness);

	text = "CMD "+std::to_string(a->myMisc->CMD);
	cv::Point position61(420,36);
	cv::putText(grayImage,text,position61,fontFace,fontScale,cv::Scalar(255),thickness);


	// doutoint = static_cast<int>(a->myMisc->gdDataAlg->llaReal[2]);
	// text = "Z "+std::to_string(doutoint);
	// cv::Point position71(420,56);
	// cv::putText(grayImage,text,position71,fontFace,fontScale,cv::Scalar(255),thickness);

	text = "ALID "+std::to_string(a->myMisc->algID);
	cv::Point position71(420,56);
	cv::putText(grayImage,text,position71,fontFace,fontScale,cv::Scalar(255),thickness);


	// doutoint = static_cast<int>(a->sysState->GdData.ph);
	// text = "PH "+std::to_string(doutoint);
	// cv::Point position81(530,16);
	// cv::putText(grayImage,text,position81,fontFace,fontScale,cv::Scalar(255),thickness);
	// doutoint = static_cast<int>(a->sysState->GdData.fy);
	// text = "FY "+std::to_string(doutoint);
	// cv::Point position91(530,36);
	// cv::putText(grayImage,text,position91,fontFace,fontScale,cv::Scalar(255),thickness);


	// doutoint = static_cast<int>(a->sysState->GdData.hg);
	// text = "GZ "+std::to_string(doutoint);
	// cv::Point position101(530,56);
	// cv::putText(grayImage,text,position101,fontFace,fontScale,cv::Scalar(255),thickness);
	// // text = "GZID "+std::to_string(a->myMisc->ct_img_id);
	text = "IMID "+std::to_string(a->myMisc->fpgaData->frame_no);
	cv::Point position111(420,76);
	cv::putText(grayImage,text,position111,fontFace,fontScale,cv::Scalar(255),thickness);





	text = "GZID "+std::to_string(a->myMisc->ct_img_id);
	cv::Point position1111(420,96);
	cv::putText(grayImage,text,position1111,fontFace,fontScale,cv::Scalar(255),thickness);
	// text = "NUM "+std::to_string(a->sysState->GdData.u8weixingshu1);
	// cv::Point position1111(420,96);
	// cv::putText(grayImage,text,position1111,fontFace,fontScale,cv::Scalar(255),thickness);
	//alg
	cv::Rect rect2(rect[0], rect[1], rect[2], rect[3]); 
	rect2 = rect2 & cv::Rect(0, 0, grayImage.cols, grayImage.rows);
	cv::rectangle(grayImage, rect2, CV_RGB(255, 255, 255), 2);
	int crossLength = 20;
	cv::line(grayImage,cv::Point(rect[0]+2,rect[1]+rect[3]/2),cv::Point(rect[0]+rect[2]-2,rect[1]+rect[3]/2),cv::Scalar(0),3);
	cv::line(grayImage,cv::Point(rect[0]+rect[2]/2,rect[1]+2),cv::Point(rect[0]+rect[2]/2,rect[1]+rect[3]-2),cv::Scalar(0),3);



//
	text = "TGAID "+std::to_string(a->sysState->PBind.u8ImgTarget_id);
	//std::string text = "ID:"+std::to_string(a->sysState->Data.u32RealDistance);
	cv::Point position1221(420,460);
	cv::putText(grayImage,text,position1221,fontFace,fontScale,cv::Scalar(255),thickness);
	text = "TEPID "+std::to_string(a->sysState->PBind.u8ImgTarget_Info_id);
	//std::string text = "ID:"+std::to_string(a->sysState->Data.u32RealDistance);
	cv::Point position1212(530,460);
	cv::putText(grayImage,text,position1212,fontFace,fontScale,cv::Scalar(255),thickness);



	


	if(a->modeAlg == 0x01){
		cv::Mat background(512,640,CV_8UC1,(void*)buffer);
		cv::Mat overlay(204,204,CV_8UC1,(void*)a->myMisc->imgData->algzj);
		cv::Mat overlayResized;
		cv::resize(overlay,overlayResized,cv::Size(102,102));
		int bx = background.cols - overlayResized.cols;
		int oy = background.rows - overlayResized.rows;
		cv::Mat roi = background(cv::Rect(bx,oy-75,102,102));
		overlayResized.copyTo(roi);

		memcpy(buffer,background.data,640*512);
	}

	unsigned char imgid[640*512/8];
	memset(imgid,0x00,640*512/8); //0无，1掩码

	doutoint = static_cast<int>(a->myMisc->Data.distance_X);
	Locate_Display((unsigned char*)imgid,doutoint,2,18,X_MASK);
	doutoint = static_cast<int>(a->myMisc->Data.distance_Y);
	Locate_Display((unsigned char*)imgid,doutoint,2,58,Y_MASK);
	doutoint = static_cast<int>(a->myMisc->Data.distance_Z);
	Locate_Display((unsigned char*)imgid,doutoint,2,98,Z_MASK);
	// 步骤3: 将掩码通过 SPI 发送
	spi_send_mask_async((unsigned char*)imgid, 640*512/8);
	
	// 等待 SPI 发送完成
	spi_wait_completion();
	
	// // 清理 SPI 控制器
	// // spi_controller_cleanup();
	
	return 0;
}

static UINT16 CheckWordCompute(unsigned char* addr, unsigned int ByteCnt)
{
	UINT16 sum = 0;
	unsigned int i = 0;
	unsigned char* ptr = addr; // 强制转换为UINT16指针，按2字节访问

	for (i = 0;i < ByteCnt;i++)
	{
		sum += (*ptr);
		ptr++;
	}
	

	return sum;
}

int getImgData(Param* a)
{
	FPGA_DATA* p;
	UINT08* buff;
	UINT16 CRC16, RCRC16;
	//static int i=0;
	buff = a->imgData->buf + (640*512*2) - 20;
	p = (FPGA_DATA*)buff;

	int i=0;
	for (i = 0;i < 20; i++) {
		DEBUG("%02x ", buff[i]);
	}
	printf("\nlong = %d\n",sizeof(UINT64));
	DEBUG("\n");

	if((p->u8FrmHeadHigh == 0xAA) && (p->u8FrmHeadLow == 0x55)){
		// 校验和
		CRC16 = CheckWordCompute(buff+2, 20 - 4);
		//RCRC16 = (rbuf[len - 1] << 8) + rbuf[len - 2];
		printf("%d %d\n",CRC16,p->u16check);
		if(p->u16check == CRC16){
			a->myMisc->fpgaData->frame_no = p->frame_no;
			a->myMisc->fpgaData->time_s = p->time_s;
		}
	}
	//a->myMisc->fpgaData->frame_no = i++;
	return 0;
}