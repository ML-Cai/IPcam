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

#include <AL/al.h>
#include <AL/alc.h> 

#include <fftw3.h>

#define __USE_GNU
#include <sched.h>

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "video.h"
#include "encoder.h"
#include "VOD_network_packet.h"
#include "../BBBIOlib/BBBio_lib/BBBiolib.h"
/* ------------------------------------------------------------ */
#define VIDEO_NET_PORT	5000
#define AUDIO_NET_PORT	5005
#define NET_HOST        "140.125.33.216"
#define NET_BUFFER_SIZE 1028
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
int FB ;
unsigned char *FB_ptr =NULL;
int FB_scerrn_size = 0;
struct fb_var_screeninfo var_info ;
struct fb_fix_screeninfo fix_info ;

unsigned char *RGB565_buffer =NULL;

struct system_information sys_info ;
pthread_t Video_Rx_thread ;
pthread_t Video_Tx_thread ;
pthread_t Audio_Tx_thread;
pthread_t Audio_Rx_thread;
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
/* Frame Buffer initial
 *
 */
int FB_init()
{
	FB = open("/dev/fb0", O_RDWR);
	if(!FB) {
		fprintf(stderr, "open /dev/fb0 error\n");
		return 0;
	}

	/* get frame buffer information  */
//	if(ioctl(FB, FBIOGET_FSCREENINFO, &fix_info)) {
//		fprintf(stderr, "FBIOGET_FSCREENINFO  error\n");
//		return 0;
//	}

	if(ioctl(FB, FBIOGET_VSCREENINFO, &var_info)) {
		fprintf(stderr, "FBIOGET_VSCREENINFO  error\n");
		return 0;
	}

	FB_scerrn_size = (var_info.xres * var_info.yres * var_info.bits_per_pixel) / 8;
	printf("frame buffer : %d %d, %dbpp\n",var_info.xres, var_info.yres, var_info.bits_per_pixel);
	printf("screensize : %d\n", FB_scerrn_size);

	FB_ptr =(unsigned char*)mmap(0, FB_scerrn_size, PROT_READ | PROT_WRITE, MAP_SHARED, FB, 0);
	if(FB_ptr == NULL){
		fprintf(stderr, "mmap error\n");
		return 0;
	}
	return FB;
}
/* ------------------------------------------------------------ */
/* Buffering data to frame buffer for display
 * 
 */
void FB_display(unsigned char *RGB565_data_ptr,int top, int left, int width, int height)
{
	/* Direct display in localhost FrameBuffer*/
	int x , y ;
	int index_monitor_1 = 0;
	int index_monitor_2 =0;
	int index_monitor_3 =0;
	int index_monitor_4 =0;
	int index_frame = 0;

	/* center picture */
	const int x_offset = (var_info.xres - width)/2 ;
	const int y_offset = ((var_info.yres - height) /2) * var_info.xres ;
	const int x_y_offset = x_offset + y_offset ;


	for(y = 0 ; y < height ; y++) {
		for(x = 0 ; x < width ; x++) {
			index_monitor_1 = (y*2 * var_info.xres + x*2) *2+ x_y_offset;
			index_monitor_2 = (y*2 * var_info.xres + (x*2+1)) *2+ x_y_offset;
			index_monitor_3 = ((y*2+1) * var_info.xres + x*2) *2+ x_y_offset;
			index_monitor_4 = ((y*2+1) * var_info.xres + (x*2+1)) *2+ x_y_offset;
			index_frame = (y * sys_info.cam.width + x) *2;

			*(FB_ptr + index_monitor_1) = *(RGB565_buffer + index_frame);
			*(FB_ptr + index_monitor_1 +1) = *(RGB565_buffer +index_frame +1);

			*(FB_ptr + index_monitor_2) = *(RGB565_buffer + index_frame);
			*(FB_ptr + index_monitor_2 +1) = *(RGB565_buffer +index_frame +1);


			*(FB_ptr + index_monitor_3) = *(RGB565_buffer + index_frame);
			*(FB_ptr + index_monitor_3 +1) = *(RGB565_buffer +index_frame +1);


			*(FB_ptr + index_monitor_4) = *(RGB565_buffer + index_frame);
			*(FB_ptr + index_monitor_4 +1) = *(RGB565_buffer +index_frame +1);
		}
	}

}
void FB_clear(int width, int height) 
{
	int x , y ;
	int index_monitor = 0;
	int index_frame = 0;
	for(y = 0 ; y < height ; y++) {
		for(x = 0 ; x < width ; x++) {
			index_monitor = (y * var_info.xres + x) *2;

			*(FB_ptr + index_monitor) =0;
			*(FB_ptr + index_monitor +1) =0;
		}
	}
}
/* ------------------------------------------------------------ */
struct vbuffer * wait_webcam_data()
{
	fd_set fds;
	struct timeval tv;
	struct vbuffer *webcam_buf;
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
static int Rx_socket_init(int *Rx_socket, struct sockaddr_in *Rx_addr, int Rx_port)
{
	/* create socket */
	if((*Rx_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Socket Failed\n");
		return 0;
	}

	/* resize Buffer size */
	int exBufferSize = 1024 * 1024 * 10 ;
	if(setsockopt(*Rx_socket ,SOL_SOCKET ,SO_RCVBUF , (char*)&exBufferSize  ,sizeof(int)) == -1) {
		fprintf(stderr, "setsockopt Failed\n");
		return 0;
	}


	/* Bind */
	Rx_addr->sin_family = AF_INET;
	Rx_addr->sin_port = htons(Rx_port);
	Rx_addr->sin_addr.s_addr = INADDR_ANY;
	if(bind(*Rx_socket, (struct sockaddr *)Rx_addr, sizeof(struct sockaddr_in))== -1) {
		fprintf(stderr, "bind error\n");
		return 0;
	}
	return 1;
}
/* ------------------------------------------------------------ */
static void *Video_Rx_loop()
{
	int Rx_socket;
	struct sockaddr_in Rx_addr;
	struct VOD_DataPacket_struct Rx_Buffer ;

	/* Socket init */
	if(Rx_socket_init(&Rx_socket, &Rx_addr, VIDEO_NET_PORT) ==0) {
		fprintf(stderr, "Rx Socket init \n");
		return NULL;
	}
	printf("Rx socket init finish\n");

	/* receive data */
	int recv_len ; 
	AVPacket packet ;
	int remain_size =0;
	unsigned char *RGB_buffer =NULL;	/* feedback pointer */
	int ID ;
	int count =0;

	while(sys_get_status() != SYS_STATUS_RELEASE) {
		/* Rx video data */
		while(sys_get_status() == SYS_STATUS_WORK) {
			recv_len = recv(Rx_socket, (char*)&Rx_Buffer, sizeof(Rx_Buffer) , 0) ;
			if(recv_len == -1) {
				fprintf(stderr ,"Stream data recv() error\n");
				break ;
			}
			else {
				switch(Rx_Buffer.DataType) {
				case VOD_PACKET_TYPE_FRAME_HEADER :	/* AVPacket as header */
					memcpy(&packet, &Rx_Buffer.header , sizeof(AVPacket));
					packet.data = (unsigned char *)malloc(sizeof(char) * packet.size);
					remain_size = packet.size;
					break ;

				case VOD_PACKET_TYPE_FRAME_DATA :	/* AVPacket.data */
					ID = Rx_Buffer.data.ID ;
					memcpy(packet.data + ID * 1024, &Rx_Buffer.data.data[0] , Rx_Buffer.data.size);

					remain_size -= Rx_Buffer.data.size;
					/* receive finish */
					if(remain_size <= 0) {
						/* Decode frame */
						video_decoder(packet.data, packet.size, &RGB_buffer);

						/* Display in Frame buffer*/
						RGB24_to_RGB565(RGB_buffer, RGB565_buffer, sys_info.cam.width, sys_info.cam.height);
						FB_display(RGB565_buffer, 0, 0, sys_info.cam.width, sys_info.cam.height);
						av_free_packet(&packet);
					}
					count ++;
					break ;
				}
			}
		}
		usleep(50000);
	}
	close(Rx_socket);
	printf("Video Rx finish\n");
	pthread_exit(NULL); 
}
/* ------------------------------------------------------------ */
/* Video Transmit loop
 *
 *	only transmit Video data , fetch data from webcam , and ecoder that as YUV420 frame .
 */
static void *Video_Tx_loop()
{
        unsigned int count = 0;


	/* init socket data*/
	int Tx_socket ;
	struct sockaddr_in Tx_addr;
	if((Tx_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Socket Faile\n");
		exit(-1);
	}
	Tx_addr.sin_family = AF_INET;
	Tx_addr.sin_port = htons(VIDEO_NET_PORT);
	Tx_addr.sin_addr.s_addr=inet_addr(NET_HOST);

	/* Webcam init */
	sys_info.cam.handle = webcam_open();

	webcam_show_info(sys_info.cam.handle);
	webcam_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.handle);
	webcam_set_framerate(sys_info.cam.handle, 15);
	webcam_start_capturing(sys_info.cam.handle);


	struct timeval t_start,t_end;
	double uTime =0.0;
	int total_size=0;
	struct vbuffer *webcam_buf;

	/* start video capture and transmit */
	printf("Video Tx\n");
	gettimeofday(&t_start, NULL);
	while(sys_get_status() != SYS_STATUS_RELEASE) {
		/* start Tx Video */
		while (sys_get_status() == SYS_STATUS_WORK) {
			webcam_buf = wait_webcam_data();
			if (webcam_buf !=NULL) {
				count++;
				struct AVPacket *pkt_ptr = video_encoder(webcam_buf->start);

				/* ------------------------------------- */
				/* direct display in localhost */
				unsigned char *RGB_buffer = NULL;
				video_decoder(pkt_ptr->data, pkt_ptr->size, &RGB_buffer);
				RGB24_to_RGB565(RGB_buffer, RGB565_buffer, sys_info.cam.width, sys_info.cam.height);
				FB_display(RGB565_buffer, 0, 0, sys_info.cam.width, sys_info.cam.height);

				total_size += pkt_ptr->size;

				/* network transmit */
				struct VOD_DataPacket_struct Tx_Buffer;

				/* Tx header */
				Tx_Buffer.DataType = VOD_PACKET_TYPE_FRAME_HEADER;
				memcpy(&Tx_Buffer.header, pkt_ptr, sizeof(AVPacket));
				sendto(Tx_socket, (char *)&Tx_Buffer, sizeof(Tx_Buffer), 0, (struct sockaddr *)&Tx_addr, sizeof(struct sockaddr_in));

				/* Tx data , 1024 bit per transmit */
				int offset =1024 ;
				int remain_size = pkt_ptr->size ;

				Tx_Buffer.DataType = VOD_PACKET_TYPE_FRAME_DATA;
				Tx_Buffer.data.ID = 0 ;
				while(remain_size > 0) {
					if (remain_size < 1024) {	/* verify transmit data size*/
						Tx_Buffer.data.size = remain_size ;
					}
					else {
						Tx_Buffer.data.size = 1024 ;
					}
					memcpy(&Tx_Buffer.data.data[0], pkt_ptr->data + Tx_Buffer.data.ID * 1024, sizeof(char) * Tx_Buffer.data.size);
					sendto(Tx_socket, (char *)&Tx_Buffer, sizeof(Tx_Buffer), 0, (struct sockaddr *)&Tx_addr, sizeof(struct sockaddr_in));
					Tx_Buffer.data.ID ++ ;
					remain_size -= Tx_Buffer.data.size ;
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
#define AUDIO_SAMPLE_RATE	44100
#define AUDIO_BUFFER_SIZE	1028
 #define AUDIO_Rx_PLAY_BUFFER_COUNT      10
static void *Audio_Tx_loop()
{
	int Tx_socket =-1;
	struct sockaddr_in dest;
	unsigned int Audio_buffer[AUDIO_SAMPLE_RATE];
	unsigned char Audio_net_buffer[AUDIO_BUFFER_SIZE] ={0};

	dest.sin_family = AF_INET;
	dest.sin_port = htons(AUDIO_NET_PORT);
	inet_aton(NET_HOST, &dest.sin_addr);
	
	Tx_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(Tx_socket < 0) {
		fprintf(stderr, "socket error\n");
		return ;
	}

	const int clk_div = 34 ;
	const int open_dly = 1;
	const int sample_dly = 1;

	BBBIO_ADCTSC_module_ctrl(BBBIO_ADC_WORK_MODE_TIMER_INT ,clk_div);
//	BBBIO_ADCTSC_module_ctrl(BBBIO_ADC_WORK_MODE_BUSY_POLLING ,clk_div);
	BBBIO_ADCTSC_channel_ctrl(BBBIO_ADC_AIN0, BBBIO_ADC_STEP_MODE_SW_CONTINUOUS, open_dly, sample_dly, BBBIO_ADC_STEP_AVG_1, Audio_buffer, AUDIO_SAMPLE_RATE);

	/* Set scheduler */
	int PID =getpid();
	struct sched_param param;
	int maxpri = sched_get_priority_max(SCHED_FIFO);
	param.sched_priority=maxpri ;
	sched_setscheduler(PID , SCHED_FIFO ,&param );
	int count =0;


	/*-------------------------*/
/*
	ALCdevice *PlayDevice ;
	ALCcontext* PlayContext ;
	ALuint PlaySource; 
	ALuint PlayBuffer[AUDIO_Rx_PLAY_BUFFER_COUNT]; 

	printf("Audio_Tx_loop\n");
	PlayDevice = alcOpenDevice(NULL);
	printf("%X\n",PlayDevice);
	PlayContext = alcCreateContext(PlayDevice, NULL); 

	alcMakeContextCurrent(PlayContext); 

	alGenSources(1, &PlaySource);
	alGenBuffers(AUDIO_Rx_PLAY_BUFFER_COUNT, PlayBuffer); 
*/
	int Bufcount =0;
	unsigned short play_Audio_buf[44100];
	int i;

	/*----------------------------*/



	printf("Audio Tx\n");
	struct timeval t_start,t_end;
	float mTime =0;
	while(sys_get_status() != SYS_STATUS_RELEASE) {
		/* start Tx Audeo */
		while(sys_get_status() == SYS_STATUS_WORK) {
			/* fetch data from ADC */
			BBBIO_ADCTSC_channel_enable(BBBIO_ADC_AIN0);
			gettimeofday(&t_start, NULL);
			BBBIO_ADCTSC_work(AUDIO_SAMPLE_RATE);
			gettimeofday(&t_end, NULL);

			/*------------------------------------------*/
/*
			for(i=0 ;i< 44100; i++){
				*(play_Audio_buf+i) = *(Audio_buffer+i) *250 ;
			}
			alBufferData(PlayBuffer[Bufcount], AL_FORMAT_MONO16, (ALvoid *)play_Audio_buf ,sizeof(short) * 44100, 44100);
			alSourcei(PlaySource, AL_BUFFER, PlayBuffer[Bufcount]); 
			alSourcePlay(PlaySource);
			Bufcount++;
			if(Bufcount ==10) Bufcount = 0;
*/
			/*------------------------------------------*/

			for(i = 0 ; i < 44100 ; i++){
				play_Audio_buf[i] = Audio_buffer[i];
			}

			mTime = (t_end.tv_sec -t_start.tv_sec)*1000000.0 +(t_end.tv_usec -t_start.tv_usec);
			mTime /=1000000.0f;
			printf("Sampling finish , fetch [%d] samples in %lfs\n", AUDIO_SAMPLE_RATE, mTime);

			int buf_size = sizeof(short) * AUDIO_SAMPLE_RATE;
			unsigned char * buf_ptr = (unsigned char *)play_Audio_buf;
			int offset =1024 ;
			int ID =0;
			int frame_size = buf_size;
			while(frame_size >0) {
				memcpy(&Audio_net_buffer[0], &ID, sizeof(int));
				memcpy(&Audio_net_buffer[4], buf_ptr + ID * 1024, sizeof(char) * offset);
				sendto(Tx_socket, &Audio_net_buffer[0], (4 + offset) , 0, (struct sockaddr *)&dest, sizeof(dest));
				frame_size -= 1024 ;
				ID ++ ;
				if(frame_size <1024) offset = frame_size ;
			}


		}
		usleep(100000);
	}
	/*------------------------------------------*/
	//alcCloseDevice(PlayDevice);
	//alcDestroyContext(PlayContext);
	//alDeleteSources(1, &PlaySource); 
	//alDeleteBuffers(AUDIO_Rx_PLAY_BUFFER_COUNT, PlayBuffer); 
	 /*------------------------------------------*/
	printf("Audio Tx finish\n");
	pthread_exit(NULL); 	
}
/* ------------------------------------------------------------ */
#define AUDIO_Rx_PLAY_BUFFER_COUNT	10
static void *Audio_Rx_loop()
{
	int Rx_socket;
	struct sockaddr_in Rx_addr;

	/* Socket init */
	if(Rx_socket_init(&Rx_socket, &Rx_addr, AUDIO_NET_PORT) ==0) {
		fprintf(stderr, "Rx Socket init \n");
		return NULL;
	}
	printf("Audio Rx socket init finish\n");

	ALCdevice *PlayDevice ;
	ALCcontext* PlayContext ;
	ALuint PlaySource; 
	ALuint PlayBuffer[AUDIO_Rx_PLAY_BUFFER_COUNT]; 

	printf("Audio Rx looping\n");

	PlayDevice = alcOpenDevice(NULL);
	printf("Rx Audio Device :%X\n",PlayDevice);
	PlayContext = alcCreateContext(PlayDevice, NULL); 
	printf("Rx Audio PlayContext :%X\n", PlayContext);

	alcMakeContextCurrent(PlayContext); 

	alGenSources(1, &PlaySource);
	alGenBuffers(AUDIO_Rx_PLAY_BUFFER_COUNT, PlayBuffer); 


	/*----------------------------*/
	/* fftw init */
	double *FT_in;
	double *IFT_in;
	fftw_complex *FT_out;
	fftw_complex *FT_out_tmp;
	fftw_plan FT_plan;
	fftw_plan IFT_plan;
	const double FOURIER_NUMBER = (floor( (double)AUDIO_SAMPLE_RATE*0.5 )+1); 

	FT_in =  (double*) fftw_malloc(sizeof(double) * (int)AUDIO_SAMPLE_RATE);
	IFT_in = (double*) fftw_malloc(sizeof(double) *(int) AUDIO_SAMPLE_RATE);
	FT_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (int)FOURIER_NUMBER);
	FT_plan =  fftw_plan_dft_r2c_1d(AUDIO_SAMPLE_RATE, FT_in, FT_out, (int)FFTW_ESTIMATE);
	IFT_plan = fftw_plan_dft_c2r_1d(AUDIO_SAMPLE_RATE, FT_out,IFT_in, (int)FFTW_ESTIMATE);


	int Buf_ID =0;
	int Buf_counter =0;
	unsigned short play_Audio_buf[AUDIO_SAMPLE_RATE];
	int recv_len;
	int ID;
	int remain_size = AUDIO_SAMPLE_RATE * sizeof(short);
	unsigned char Rx_Buffer[AUDIO_BUFFER_SIZE];
	int i;
	while(sys_get_status() != SYS_STATUS_RELEASE) {
		/* start Tx Audeo */
		printf("Audio Rx 1\n");
		while(sys_get_status() == SYS_STATUS_WORK) {
			recv_len = recv(Rx_socket, (char*)&Rx_Buffer, sizeof(Rx_Buffer) , 0) ;
			
			if(recv_len == -1) {
				fprintf(stderr ,"Stream data recv() error\n");
				break ;
			}
			else {
				ID = *((int *)Rx_Buffer) ;
				memcpy((char *)play_Audio_buf + ID * 1024, &Rx_Buffer[4] , recv_len-4);
				remain_size -= (recv_len-4);

				/* receive finish */
				if(remain_size <= 0) {
					for(i = 0; i < AUDIO_SAMPLE_RATE; i++) {
						*(FT_in + i) = play_Audio_buf[i] * 160;
					}

					fftw_execute(FT_plan);

					for(i = 0 ; i < 400 ; i++) {
						FT_out[i][0] = 0;
						FT_out[i][1] = 0;
					}
					for(i = 6000 ; i < FOURIER_NUMBER ; i++) {
						FT_out[i][0] = 0;
						FT_out[i][1] = 0;
					}

					fftw_execute(IFT_plan);

					for(i = 0; i < AUDIO_SAMPLE_RATE; i++) {
						play_Audio_buf[i] = (IFT_in[i] /AUDIO_SAMPLE_RATE) *60.0f;
					}


					/* Decode frame */
					printf("Play\n");
					alSourceStop(PlaySource);
					alBufferData(PlayBuffer[Buf_counter], AL_FORMAT_MONO16, (ALvoid *)play_Audio_buf ,sizeof(short) * 44100, 44100);
					alSourcei(PlaySource, AL_BUFFER, PlayBuffer[Buf_counter]); 
					alSourcePlay(PlaySource);
					Buf_counter++;
					if(Buf_counter >= 10) Buf_counter = 0;
					remain_size = AUDIO_SAMPLE_RATE * sizeof(short);
				}
			}
		}
		usleep(50000);
	}
	/*------------------------------------------*/
	alcCloseDevice(PlayDevice);
	alcDestroyContext(PlayContext);
	alDeleteSources(1, &PlaySource); 
	alDeleteBuffers(AUDIO_Rx_PLAY_BUFFER_COUNT, PlayBuffer); 
	 /*------------------------------------------*/
	printf("Audio Rx finish\n");
}
/* ------------------------------------------------------------ */
void SIGINT_release(int arg)
{
	sys_set_status(SYS_STATUS_RELEASE);
	printf("System Relase ... \n");
}
/* ------------------------------------------------------------ */
static void list_audio_devices(const ALCchar *devices)
{
        const ALCchar *device = devices, *next = devices + 1;
        size_t len = 0;

        fprintf(stdout, "Devices list:\n");
        fprintf(stdout, "----------------------\n");
        while (device && *device != '\0' && next && *next != '\0') {
                fprintf(stdout, "<%s>\n", device);
                len = strlen(device);
                device += (len + 1);
                next += (len + 2);
        }
        fprintf(stdout, "-----------------------\n");
}
/* ------------------------------------------------------------ */
int main()
{
	list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));
	/* BBBIOlib init*/
	iolib_init();
	iolib_setdir(8,11, BBBIO_DIR_IN);	/* Button */
	iolib_setdir(8,12, BBBIO_DIR_OUT);	/* LED */


//	sys_info.capability = SYS_CAPABILITY_VIDEO_Tx | SYS_CAPABILITY_VIDEO_Rx | SYS_CAPABILITY_AUDIO_Tx | SYS_CAPABILITY_AUDIO_Rx;
//	sys_info.capability = SYS_CAPABILITY_VIDEO_Tx | SYS_CAPABILITY_AUDIO_Tx;
	sys_info.capability =SYS_CAPABILITY_AUDIO_Tx | SYS_CAPABILITY_AUDIO_Rx;
//	sys_info.capability = SYS_CAPABILITY_VIDEO_Tx;

	sys_info.status = SYS_STATUS_INIT;
	sys_info.cam.width = 320;
	sys_info.cam.height = 240;
//	sys_info.cam.pixel_fmt = V4L2_PIX_FMT_YUV420;
	sys_info.cam.pixel_fmt = V4L2_PIX_FMT_YUYV;

	/* alloc RGB565 buffer for frame buffer data store */
	RGB565_buffer = (unsigned char *)malloc(sys_info.cam.width * sys_info.cam.height *2);

	/* step Codec register  */
	avcodec_register_all();
	av_register_all();
	video_encoder_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.pixel_fmt);
	video_decoder_init(sys_info.cam.width, sys_info.cam.height, sys_info.cam.pixel_fmt);
	printf("Codec init finish\n");

	/* step Frame buffer initial*/
	if(FB_init() == 0) {
		fprintf(stderr, "Frame Buffer init error\n");
	}
	FB_clear(var_info.xres, var_info.yres);

	sys_set_status(SYS_STATUS_IDLE);

	/* Create Video thread */
	if(sys_info.capability & SYS_CAPABILITY_VIDEO_Rx) pthread_create(&Video_Rx_thread, NULL, Video_Rx_loop, NULL);
	if(sys_info.capability & SYS_CAPABILITY_VIDEO_Tx) pthread_create(&Video_Tx_thread, NULL, Video_Tx_loop, NULL);

	/* Create Audio thread*/
	if(sys_info.capability & SYS_CAPABILITY_AUDIO_Rx) pthread_create(&Audio_Rx_thread, NULL, Audio_Rx_loop, NULL);
	if(sys_info.capability & SYS_CAPABILITY_AUDIO_Tx) pthread_create(&Audio_Tx_thread, NULL, Audio_Tx_loop, NULL);

	/* Signale SIGINT */
	signal(SIGINT, SIGINT_release);

	/* Main loop */
	while(sys_get_status() != SYS_STATUS_RELEASE) {
		/* Button on */
		if (is_high(8,11)) {
			sys_set_status(SYS_STATUS_WORK);
			pin_high(8, 12);	/* LED on*/
		}
		else {
//			FB_clear(var_info.xres, var_info.yres);
//			sys_set_status(SYS_STATUS_IDLE);
			sys_set_status(SYS_STATUS_WORK);
			pin_low(8, 12);	/* LED off */
		}
		//usleep(100000);
		sleep(1);
	}
	pin_low(8, 12); /* LED off */

	/* *******************************************************
	 * Main thread for SIP server communication and HW process
	 *
	 *
	 * *******************************************************/

	/* release */
	if(sys_info.capability & SYS_CAPABILITY_VIDEO_Tx) pthread_join(Video_Tx_thread,NULL);
//	if(sys_info.capability & SYS_CAPABILITY_VIDEO_Rx) pthread_join(Video_Rx_thread,NULL);
	if(sys_info.capability & SYS_CAPABILITY_AUDIO_Tx) pthread_join(Audio_Tx_thread,NULL);

	munmap(FB_ptr, FB_scerrn_size);
	close(FB);
	free(RGB565_buffer);

	video_encoder_release();
	video_decoder_release();
	printf("finish\n");
	return 0;
}
