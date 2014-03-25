#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <linux/fb.h>

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "video.h"
#include "encoder.h"
#include "VOD_network_packet.h"
/* ------------------------------------------------------------ */
#define NET_PORT        5000
#define NET_HOST        ""
#define NET_BUFFER_SIZE 1028
/* ------------------------------------------------------------ */
struct system_information
{
	struct webcam_info cam;
	int left ;
	int top ;
};
/* ------------------------------------------------------------ */
int FB ;
unsigned char *FB_ptr =NULL;
struct fb_var_screeninfo var_info ;
struct fb_fix_screeninfo fix_info ;

//unsigned char RGB565_buffer[240][320][2];
unsigned char *RGB565_buffer =NULL;

struct system_information sys_info ;

/* ------------------------------------------------------------ */
int wait_webcam_data()
{
}
/* ------------------------------------------------------------ */
static void mainloop()
{
        unsigned int count = 300;

	int Tx_socket ;
	struct sockaddr_in Tx_addr;
	//char Buffer[NET_BUFFER_SIZE] ={0};
	if((Tx_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Socket Faile\n");
		exit(-1);
	}
	Tx_addr.sin_family = AF_INET;
	Tx_addr.sin_port = htons(NET_PORT);
	Tx_addr.sin_addr.s_addr=inet_addr(NET_HOST);
	//inet_aton(NET_HOST, &Tx_addr.sin_addr);

	struct timeval t_start,t_end;
	double uTime =0.0;
	int total_size=0;
	gettimeofday(&t_start, NULL);


	struct vbuffer *webcam_buf;
	while (count-- > 0) {
		fd_set fds;
		struct timeval tv;
		int r;

		/* add in select set */
		FD_ZERO(&fds);
		FD_SET(sys_info.cam.handle, &fds);

		/* Timeout. */
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		r = select(sys_info.cam.handle + 1, &fds, NULL, NULL, &tv);

		if (r == -1) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		if (r == 0) {
			perror("select timeout\n");
			exit(EXIT_FAILURE);
		}
		webcam_buf = webcam_read_frame(sys_info.cam.handle);
	//	continue;
		if (webcam_buf !=NULL) {
			int offset =1024 ;
			int id =0;
			struct AVPacket *pkt_ptr = video_encoder(webcam_buf->start);

			/* ------------------------------------- */
			/* decode for test*/
			unsigned char *RGB_buffer =NULL;
			
			/* direct display in localhost */
			/*
			video_decoder(pkt_ptr->data, pkt_ptr->size, &RGB_buffer);
			RGB24_to_RGB565(RGB_buffer, RGB565_buffer, sys_info.cam.width, sys_info.cam.height);
			int x , y ;
			int index_monitor = 0;
			int index_frame = 0;

			for(y = 0 ; y < sys_info.cam.height ; y++) {
				for(x = 0 ; x < sys_info.cam.width ; x++) {
					index_monitor = (y * 1280 + x) *2;
					index_frame = (y * sys_info.cam.width + x) *2;
					*(FB_ptr + index_monitor) = *(RGB565_buffer + index_frame);
					*(FB_ptr + index_monitor +1) = *(RGB565_buffer +index_frame +1);
				}
			}
			*/


			/* network transmit */
			//struct AVPacket *pkt_ptr = video_encoder(buffer.start);
			struct VOD_DataPacket_struct Tx_Buffer;
			total_size +=pkt_ptr->size;

			/* Tx header */
			Tx_Buffer.DataType = VOD_PACKET_TYPE_FRAME_HEADER;
			memcpy(&Tx_Buffer.header ,pkt_ptr ,sizeof(AVPacket));
			sendto(Tx_socket, (char *)&Tx_Buffer, sizeof(Tx_Buffer), 0, (struct sockaddr *)&Tx_addr, sizeof(struct sockaddr_in));

			/* Tx data */
			int slice =0;
			int remain_size = pkt_ptr->size ;

			while(remain_size > 0) {
				Tx_Buffer.DataType = VOD_PACKET_TYPE_FRAME_DATA;
				Tx_Buffer.data.ID = slice ;
				if (remain_size < 1024) {
					memcpy(&Tx_Buffer.data.data[0], pkt_ptr->data + slice * 1024, sizeof(char)*remain_size);
					Tx_Buffer.data.size = remain_size ;
				}
				else {
					memcpy(&Tx_Buffer.data.data[0], pkt_ptr->data + slice * 1024, sizeof(char)*1024);
					Tx_Buffer.data.size = 1024 ;
				}
				sendto(Tx_socket, (char *)&Tx_Buffer, sizeof(Tx_Buffer), 0, (struct sockaddr *)&Tx_addr, sizeof(struct sockaddr_in));
				slice ++ ;
				remain_size -= 1024 ;
			}
			/* ------------------------------------- */
		}
		else {
			printf("webcam_read_frame error\n");
		}
		/* EAGAIN - continue select loop. */
	}
	gettimeofday(&t_end, NULL);
	uTime = (t_end.tv_sec -t_start.tv_sec)*1000000.0 +(t_end.tv_usec -t_start.tv_usec);
	printf("Total size :%d bit\n", total_size);
	printf("Time :%lf us\n", uTime);
	printf("kb/s %lf\n",total_size/ (uTime/1000.0));
}
/* ------------------------------------------------------------ */
int main()
{
	FB = open("/dev/fb0", O_RDWR);
	if(!FB) {
		fprintf(stderr, "open /dev/fb0 error\n");
		return -1;
	}

	if(ioctl(FB, FBIOGET_FSCREENINFO, &fix_info)) {
		fprintf(stderr, "FBIOGET_FSCREENINFO  error\n");
		return -1;
	}

	if(ioctl(FB, FBIOGET_VSCREENINFO, &var_info)) {
		fprintf(stderr, "FBIOGET_VSCREENINFO  error\n");
		return -1;
	}


	printf("frame buffer : %d %d, %dbpp\n",var_info.xres, var_info.yres, var_info.bits_per_pixel);
	long int screensize = (var_info.xres*var_info.yres*var_info.bits_per_pixel )/8;
	printf("screensize : %d\n",screensize);


	FB_ptr =(unsigned char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, FB, 0);
	if(FB_ptr ==NULL){
		fprintf(stderr, "mmap error\n");
		return -1;
	}


	/* -------------------------------------- */
	sys_info.cam.width = 320;
	sys_info.cam.height = 240;
//	sys_info.cam.pixel_fmt = V4L2_PIX_FMT_YUV420;
	sys_info.cam.pixel_fmt = V4L2_PIX_FMT_YUYV;
//	sys_info.cam.pixel_fmt = V4L2_PIX_FMT_MJPEG;

	RGB565_buffer = (unsigned char *)malloc(sys_info.cam.width * sys_info.cam.height *2);

	/* step Codec register  */
	avcodec_register_all();
	av_register_all();
	video_encoder_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.pixel_fmt);
	video_decoder_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.pixel_fmt);
	printf("Codec init finish\n");

	/* step Webcam init */
	sys_info.cam.handle = webcam_open();

	webcam_show_info(sys_info.cam.handle);
	webcam_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.handle);
//	webcam_set_framerate(sys_info.cam.handle, 20);
	webcam_start_capturing(sys_info.cam.handle);

	mainloop();

	munmap(FB_ptr, screensize);
	close(FB);
	free(RGB565_buffer);

	webcam_stop_capturing(sys_info.cam.handle);
	webcam_release(sys_info.cam.handle);
	video_encoder_release();
	video_decoder_release();
	close(sys_info.cam.handle);
}
