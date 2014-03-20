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

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "video.h"

/* ------------------------------------------------------------ */
#define NET_PORT        5000
#define NET_HOST        ""
#define NET_BUFFER_SIZE 1028
/* ------------------------------------------------------------ */
extern struct vbuffer buffer ;
extern int cam ;
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
		tv.tv_sec = 1;
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
	open_device();

	avcodec_register_all();
	av_register_all();
	video_encoder_init();

	printf("OK\n");

	show_device_info();
	init_device();
	start_capturing();

	mainloop();

	video_encoder_release();
	stop_capturing();
	release_device();
	close(cam);
}
