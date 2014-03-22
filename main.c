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

/* ------------------------------------------------------------ */
#define NET_PORT        5000
#define NET_HOST        "140.125.33.214"
#define NET_BUFFER_SIZE 1028
/* ------------------------------------------------------------ */
extern struct vbuffer buffer ;
extern int cam ;

int FB ;
unsigned char *FB_ptr =NULL;
struct fb_var_screeninfo var_info ;
struct fb_fix_screeninfo fix_info ;


unsigned char RGB565_buffer[240][320][2];

/* ------------------------------------------------------------ */
static void mainloop()
{
        unsigned int count = 100;

	int s_socket ;
	struct sockaddr_in dest;
	char Buffer[NET_BUFFER_SIZE] ={0};
	if((s_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Socket Faile\n");
		exit(-1);
	}
	dest.sin_family = AF_INET;
	dest.sin_port = htons(NET_PORT);
	inet_aton(NET_HOST, &dest.sin_addr);

	struct timeval t_start,t_end;
	double uTime =0.0;
	int total_size=0;
	gettimeofday(&t_start, NULL);

	while (count-- > 0) {
		fd_set fds;
		struct timeval tv;
		int r;

		/* add in select set */
		FD_ZERO(&fds);
		FD_SET(cam, &fds);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(cam+1, &fds, NULL, NULL, &tv);

		if (r == -1) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		if (r == 0) {
			perror("select timeout\n");
			exit(EXIT_FAILURE);
		}

		if (read_frame()) {
			int offset =1024 ;
			int id =0;
			unsigned char * buf_ptr = NULL;
			int frame_size = video_encoder(buffer.start, &buf_ptr);
			total_size +=frame_size;
//			printf("frame_size %d\n", frame_size);

			/* ------------------------------------- */
			/* decode for test*/
			unsigned char *RGB_buffer =NULL;
			printf("1\n");


			if (frame_size >0) {

				printf("encoder\n");
				video_decoder(buf_ptr, frame_size, &RGB_buffer);


				printf("fmt\n");
				RGB24_to_RGB565(RGB_buffer, RGB565_buffer, 320, 240);

				//memset(FB_ptr, 0, sizeof(char)* 320*240 *2);
				//memcpy(FB_ptr, RGB565_buffer, sizeof(char)* 320*240 *2);
				int x , y ;
				int index =0;
				for(y = 0 ; y < 240 ; y++) {
					for(x = 0 ; x < 320 ; x++) {
						index = (y * 1280 + x) *2;
						*(FB_ptr + index) = RGB565_buffer[y][x][0];
						*(FB_ptr + index+1) = RGB565_buffer[y][x][1];

					}
				}
				printf("OK\n");

			}

			/* ------------------------------------- */

/*

			memcpy(&Buffer[0], &id, sizeof(int));
			memcpy(&Buffer[4], &frame_size, sizeof(int));
			sendto(s_socket, &Buffer[0], 8, 0, (struct sockaddr *)&dest, sizeof(dest));
			id ++;
			while(frame_size >0) {
				memcpy(&Buffer[0], &id, sizeof(int));
				memcpy(&Buffer[4], buf_ptr + (id - 1) * 1024, sizeof(char) * offset);
				sendto(s_socket, &Buffer[0], (4+offset) , 0, (struct sockaddr *)&dest, sizeof(dest));
				frame_size -= 1024 ;
				id ++ ;
				if(frame_size <1024) offset = frame_size ;
			}
*/

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


/*
	int x ,y ,index;
	unsigned char max ;
	unsigned char R =0 , G =0, B=0;

	short *pen = NULL ;
	short RGB565 ;
	printf("%d\n", sizeof(short));
	for(y = 0 ; y < 200 ; y ++) {
		for(x = 0 ; x < 200 ; x ++) {
			index = (y * var_info.xres + x) *2 ;
			pen = (short *)(FB_ptr + index) ;

			R =255 ;
			G =255;
			B =0;

			*pen = ((R >>3) <<11) | ((G>>2) << 5) | (B>>3);
			//printf(" %d %d %d , %d\n",R ,G ,B ,*pen);

//			*(FB_ptr + index) = 255;
//			*(FB_ptr + index +1) = 255;
		}
	}

	munmap(FB_ptr, screensize);
	close(FB);
*/
	/* -------------------------------------- */



	open_device();

	avcodec_register_all();
	av_register_all();
	video_encoder_init();
	video_decoder_init();
	printf("OK\n");

	show_device_info();
	init_device();
	start_capturing();

	mainloop();

	munmap(FB_ptr, screensize);
	close(FB);

	video_encoder_release();
	video_decoder_release();
	stop_capturing();
	release_device();
	close(cam);
}
