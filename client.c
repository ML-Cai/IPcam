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
extern struct vbuffer buffer ;

int FB ;
unsigned char *FB_ptr =NULL;
struct fb_var_screeninfo var_info ;
struct fb_fix_screeninfo fix_info ;

//unsigned char RGB565_buffer[240][320][2];
unsigned char *RGB565_buffer =NULL;

struct system_information sys_info ;

/* ------------------------------------------------------------ */
static void Rx_loop()
{
	int Rx_socket ;
	struct sockaddr_in Rx_addr;
	struct VOD_DataPacket_struct Rx_Buffer ;
	/* create socket */
	if((Rx_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Socket Faile\n");
		return ;
	}

	/* Bind */
	Rx_addr.sin_family = AF_INET;
	Rx_addr.sin_port = htons(NET_PORT);
	Rx_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(Rx_socket, (struct sockaddr *)&Rx_addr, sizeof(struct sockaddr_in))== -1) {
		fprintf(stderr, "bind error\n");
		return ;
	}

	/* receive data */
	int recv_len ; 
	AVPacket packet ;
	int remain_size =0;
	unsigned char *RGB_buffer =NULL;	/* feedback pointer */
	int ID ;

	while(1) {
		recv_len = recv(Rx_socket, (char*)&Rx_Buffer, sizeof(Rx_Buffer) , 0) ;
		if(recv_len == -1) {
			fprintf(stderr ,"Stream data recv() error\n");
			return ;
		}
		else {
			switch(Rx_Buffer.DataType){
			case VOD_PACKET_TYPE_FRAME_HEADER :
				memcpy(&packet, &Rx_Buffer.header , sizeof(AVPacket));
				packet.data = (unsigned char *)malloc(sizeof(char) * packet.size);
				remain_size = packet.size;
				break ;

			case VOD_PACKET_TYPE_FRAME_DATA :
				ID =Rx_Buffer.data.ID ;
				memcpy(packet.data + ID * 1024, &Rx_Buffer.data.data[0] , Rx_Buffer.data.size);

				remain_size -= Rx_Buffer.data.size;
				if(remain_size <= 0) {	/* receive finish */
					//printf("Decode\n");
					video_decoder(packet.data, packet.size, &RGB_buffer);
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

				}
				break ;
			}
		}
	}
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

	RGB565_buffer = (unsigned char *)malloc(sys_info.cam.width * sys_info.cam.height *2);

	/* step Codec register  */
	avcodec_register_all();
	av_register_all();
	video_encoder_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.pixel_fmt);
	video_decoder_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.pixel_fmt);
	printf("Codec init finish\n");

	/* step Webcam init */
//	sys_info.cam.handle = webcam_open();
//	webcam_show_info(sys_info.cam.handle);
//	webcam_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.handle);
//	webcam_start_capturing(sys_info.cam.handle);

	Rx_loop();

	munmap(FB_ptr, screensize);
	close(FB);
	free(RGB565_buffer);

//	webcam_stop_capturing(sys_info.cam.handle);
//	webcam_release(sys_info.cam.handle);
	video_encoder_release();
	video_decoder_release();
	close(sys_info.cam.handle);
}
