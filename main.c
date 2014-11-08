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
#include <signal.h>
#include <pthread.h>

#define __USE_GNU
#include <sched.h>

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "video.h"
#include "encoder.h"
#include "network_packet.h"
/* ------------------------------------------------------------ */
#define VIDEO_NET_PORT	5000
#define AUDIO_NET_PORT	5005
#define NET_HOST        "140.125.33.214"	/* <<<<< specify your Host IP here */
/* ------------------------------------------------------------ */
#define SYS_STATUS_INIT	1
#define SYS_STATUS_IDLE	2
#define SYS_STATUS_WORK	4
#define SYS_STATUS_RELEASE	8

#define SYS_CAPABILITY_VIDEO_Tx	0x1
#define SYS_CAPABILITY_VIDEO_Rx	0x2
#define SYS_CAPABILITY_AUDIO_Tx	0x4
#define SYS_CAPABILITY_AUDIO_Rx	0x8

struct system_information
{
	int status ;
	int capability;
	struct webcam_info cam;
	int left ;
	int top ;
};

/* ------------------------------------------------------------ */
struct system_information sys_info ;
pthread_t Video_Tx_thread ;
pthread_mutex_t threadcount = PTHREAD_MUTEX_INITIALIZER;
/* ------------------------------------------------------------ */
/* Get System status 
 *	using Mutex to ensure all thread access successful
 */
int sys_get_status()
{
	int ret ;
	pthread_mutex_lock(&threadcount);
	ret = sys_info.status;
	pthread_mutex_unlock(&threadcount);
	return ret ;
}
void sys_set_status(int status)
{
	pthread_mutex_lock(&threadcount);
	sys_info.status = status;
	pthread_mutex_unlock(&threadcount);
}
/* ------------------------------------------------------------ */
struct vbuffer * wait_webcam_data()
{
	fd_set fds;
	struct timeval tv;
	struct vbuffer *webcam_buf =NULL;
	int r;

	/* add in select set */
	FD_ZERO(&fds);
	FD_SET(sys_info.cam.handle, &fds);

	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	r = select(sys_info.cam.handle + 1, &fds, NULL, NULL, &tv);

	if (r == -1) {
		fprintf(stderr, "select");
		return NULL;
	}
	if (r == 0) {
		fprintf(stderr, "select timeout\n");
		return NULL;
	}
	webcam_buf = webcam_read_frame(sys_info.cam.handle);

	return webcam_buf;
}
/* ------------------------------------------------------------ */
/* Video Transmit loop
 *
 *	only transmit Video data , fetch data from webcam , and ecoder that as YUV420 frame .
 */
static void *Video_Tx_loop()
{
	unsigned int count = 0;

	/* init socket data (UDP)*/
	int Tx_socket ;
	struct sockaddr_in Tx_addr;
	if((Tx_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket Faile\n");
		return NULL ;
	}
	Tx_addr.sin_family = AF_INET;
	Tx_addr.sin_port = htons(VIDEO_NET_PORT);
	Tx_addr.sin_addr.s_addr = inet_addr(NET_HOST);

	/* Webcam init 
	 *		@note :
	 *			Camera device : Logitech C615
	 *			Width : 800
	 *			Height : 600
	 *			Capture format : YUYV
	 *			Frame rate : 5 frame/s
	 */
	sys_info.cam.width = 640;
	sys_info.cam.height = 480;
	sys_info.cam.pixel_fmt = V4L2_PIX_FMT_YUYV;
	sys_info.cam.handle = webcam_open();
	if(sys_info.cam.handle == 0)
		return 0;
	
	webcam_show_info(sys_info.cam.handle);
	webcam_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.handle);
	webcam_set_framerate(sys_info.cam.handle, 5);
	webcam_start_capturing(sys_info.cam.handle);

	/* Encoder init */
	video_encoder_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.pixel_fmt, sys_info.cam.width, sys_info.cam.height);

	struct timeval t_start, t_end;
	double uTime =0.0;
	int total_size=0;
	struct vbuffer *webcam_buf;
	struct thumb_AVPacket Tx_pkt;

	/* start video capture and transmit */
	printf("Video Tx Looping\n");
	gettimeofday(&t_start, NULL);
	while(sys_get_status() != SYS_STATUS_RELEASE) {
		/* start Tx Video */
		while (sys_get_status() == SYS_STATUS_WORK) {
			webcam_buf = wait_webcam_data();
			if (webcam_buf !=NULL) {
				count++;
				struct AVPacket *pkt_ptr = video_encoder(webcam_buf->start);

				/* ******************************************************
				After video_encoder , pkt_ptr store the current frame .

				if you don'n want to using the network Tx code below , please note :

				The important data member in AVPacket are list in thumb_AVPacket . 

				Using network transmit the member in AVPacket same as thumb_AVPacket to the host .

				and transmit the data in AVPacket.data to your host , the data size was record in AVPacket.size

				the network Tx  code below are not necessary .

				********************************************************* */

				/* copy AVPacket data to Tx_pkt struct . (to consistent with 64 bits and 32 bits platform for data pointer) */
				Tx_pkt.pts = pkt_ptr->pts;
				Tx_pkt.dts = pkt_ptr->dts;
				Tx_pkt.size = pkt_ptr->size;
				Tx_pkt.stream_index = pkt_ptr->stream_index;
				Tx_pkt.flags = pkt_ptr->flags;
				Tx_pkt.side_data_elems = pkt_ptr->side_data_elems;
				Tx_pkt.duration = pkt_ptr->duration;
				Tx_pkt.pos = pkt_ptr->pos;
				Tx_pkt.convergence_duration = pkt_ptr->convergence_duration;

				total_size += pkt_ptr->size;

				/* network transmit */
				struct StreamPacket_struct Tx_Buffer;

				/* Tx header */
				Tx_Buffer.DataType = PACKET_TYPE_FRAME_HEADER;
				memcpy(&Tx_Buffer.header, &Tx_pkt, sizeof(struct thumb_AVPacket));

				sendto(Tx_socket, (char *)&Tx_Buffer, sizeof(Tx_Buffer), 0, (struct sockaddr *)&Tx_addr, sizeof(struct sockaddr_in));

				/* Tx data , 1024 bit per transmit */
				int offset =1024 ;
				int remain_size = pkt_ptr->size ;

				Tx_Buffer.DataType = PACKET_TYPE_FRAME_DATA;
				Tx_Buffer.data_slice.ID = 0 ;
				while(remain_size > 0) {
					if (remain_size < 1024) {	/* verify transmit data size*/
						Tx_Buffer.data_slice.size = remain_size ;
					}
					else {
						Tx_Buffer.data_slice.size = 1024 ;
					}
					memcpy(&Tx_Buffer.data_slice.data[0], pkt_ptr->data + Tx_Buffer.data_slice.ID * 1024, sizeof(char) * Tx_Buffer.data_slice.size);
					sendto(Tx_socket, (char *)&Tx_Buffer, sizeof(Tx_Buffer), 0, (struct sockaddr *)&Tx_addr, sizeof(struct sockaddr_in));
					Tx_Buffer.data_slice.ID ++ ;
					remain_size -= Tx_Buffer.data_slice.size ;
				}
			}
			else {
				printf("webcam_read_frame error\n");
			}
		}
		usleep(50000);
	}
	gettimeofday(&t_end, NULL);

	uTime = (t_end.tv_sec -t_start.tv_sec)*1000000.0 +(t_end.tv_usec -t_start.tv_usec);
	printf("Total size : %d bit , frame count : %d\n", total_size, count);
	printf("Time :%lf us\n", uTime);
	printf("kb/s %lf\n",total_size/ (uTime/1000.0));

	webcam_stop_capturing(sys_info.cam.handle);
	webcam_release(sys_info.cam.handle);
	close(sys_info.cam.handle);
	pthread_exit(NULL); 
}
/* ------------------------------------------------------------ */
void SIGINT_release(int arg)
{
	sys_set_status(SYS_STATUS_RELEASE);
	printf("System Relase ... \n");
}
/* ------------------------------------------------------------ */
int main()
{
	sys_info.capability = SYS_CAPABILITY_VIDEO_Tx;

	sys_info.status = SYS_STATUS_INIT;

	/* setup Codec register  */
	avcodec_register_all();
	av_register_all();

	sys_set_status(SYS_STATUS_IDLE);

	/* Create Video thread */
	if(sys_info.capability & SYS_CAPABILITY_VIDEO_Tx) pthread_create(&Video_Tx_thread, NULL, Video_Tx_loop, NULL);

	/* Signale SIGINT for release data */
	signal(SIGINT, SIGINT_release);

	/* Main loop */
//	while(sys_get_status() != SYS_STATUS_RELEASE) {
		sys_set_status(SYS_STATUS_WORK);
//		sleep(1);
//	}

	/* release */
	if(sys_info.capability & SYS_CAPABILITY_VIDEO_Tx) pthread_join(Video_Tx_thread,NULL);

	video_encoder_release();
	return 0;
}
