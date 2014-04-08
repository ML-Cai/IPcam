#include <stdio.h>
#include <stdlib.h>
#include <linux/videodev2.h>

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "encoder.h"
#include "format.h"
/* ------------------------------------------------------------ */
static int CAMERA_WIDTH =0;
static int CAMERA_HEIGHT =0;
/* ------------------------------------------------------------ */
struct encoder_struct VOD_encoder ;
//unsigned char YUV420P_buf[CAMERA_HEIGHT][CAMERA_WIDTH][2] ={0};
unsigned char *YUV420P_buf =NULL ;
/* ------------------------------------------------------------ */
void video_encoder_init(int width, int height, int pixel_fmt)
{
	CAMERA_WIDTH = width;
	CAMERA_HEIGHT = height;

//	M_emcoder.codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	VOD_encoder.codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
//	VOD_encoder.codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);


	if (!VOD_encoder.codec) {
		perror("codec not found\n");
		goto ERROR_EXIT;
	}

	VOD_encoder.c_context= avcodec_alloc_context3(VOD_encoder.codec);
	VOD_encoder.frame_YUV420P= av_frame_alloc();

	/* put sample parameters */
	VOD_encoder.c_context->bit_rate = CAMERA_WIDTH * CAMERA_HEIGHT * 4;

	/* resolution must be a multiple of two */
	VOD_encoder.c_context->width = CAMERA_WIDTH;
	VOD_encoder.c_context->height = CAMERA_HEIGHT;
	printf("encoder %d %d\n",VOD_encoder.c_context->width ,VOD_encoder.c_context->height);
	/* frames per second */
	VOD_encoder.c_context->time_base.num = 1 ;
	VOD_encoder.c_context->time_base.den = 20;
	VOD_encoder.c_context->gop_size = 60; /* emit one intra frame every ten frames */
	VOD_encoder.c_context->max_b_frames = 0;
	VOD_encoder.c_context->thread_count = 4;
	VOD_encoder.c_context->pix_fmt = PIX_FMT_YUV420P;

	/* open it */
	if (avcodec_open2(VOD_encoder.c_context, VOD_encoder.codec, NULL) < 0) {
		perror("could not open codec\n");
		goto ERROR_EXIT;
	}

	/* prepare Codec buffer */
	VOD_encoder.buffer_size = CAMERA_WIDTH * CAMERA_HEIGHT *3;
	VOD_encoder.buffer = (unsigned char *)malloc(VOD_encoder.buffer_size);
	av_init_packet(&VOD_encoder.pkt);
	VOD_encoder.pkt.data = VOD_encoder.buffer;
	VOD_encoder.pkt.size = VOD_encoder.buffer_size;


	YUV420P_buf = (unsigned char *)malloc(CAMERA_HEIGHT * CAMERA_WIDTH * 2);
	memset(YUV420P_buf, 0, CAMERA_HEIGHT * CAMERA_WIDTH * 2);
	VOD_encoder.picture_buf = YUV420P_buf;
	VOD_encoder.frame_YUV420P->data[0] = VOD_encoder.picture_buf;
	VOD_encoder.frame_YUV420P->data[1] = VOD_encoder.frame_YUV420P->data[0] + (CAMERA_WIDTH * CAMERA_HEIGHT);
	VOD_encoder.frame_YUV420P->data[2] = VOD_encoder.frame_YUV420P->data[1] + (CAMERA_WIDTH * CAMERA_HEIGHT) / 4;

	VOD_encoder.frame_YUV420P->linesize[0] = VOD_encoder.c_context->width;
	VOD_encoder.frame_YUV420P->linesize[1] = VOD_encoder.c_context->width / 2;
	VOD_encoder.frame_YUV420P->linesize[2] = VOD_encoder.c_context->width / 2;

	/* prepare image conveter */
	switch(pixel_fmt) {
	case V4L2_PIX_FMT_YUV420 :
	        VOD_encoder.img_convert_ctx = sws_getContext( CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P, //PIX_FMT_YUYV422,
                        	                        CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
                	                                SWS_POINT, NULL, NULL, NULL);
		break ;
	case V4L2_PIX_FMT_YUYV :
		VOD_encoder.img_convert_ctx = sws_getContext( CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUYV422, //PIX_FMT_YUYV422,
							CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
							SWS_POINT, NULL, NULL, NULL);
		break ;
	case V4L2_PIX_FMT_MJPEG :
		printf("<<set>>\n"); 
		VOD_encoder.img_convert_ctx = sws_getContext( CAMERA_WIDTH, CAMERA_HEIGHT, AV_PIX_FMT_YUVJ422P,
								CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,
								SWS_POINT, NULL, NULL, NULL);
	case PIX_FMT_RGB565LE :
		printf("<<PIX_FMT_RGB565LE, %d %d>>\n", CAMERA_WIDTH, CAMERA_HEIGHT); 
	        VOD_encoder.img_convert_ctx = sws_getContext( CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_RGB565LE, //PIX_FMT_YUYV422,
                        	                        CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
							SWS_POINT, NULL, NULL, NULL);
	}
	
	if(VOD_encoder.img_convert_ctx == NULL) {
		fprintf(stderr, "Cannot initialize the conversion context!\n");
		goto ERROR_EXIT;
	}

	printf("Encoder init finish\n");

	return ;

ERROR_EXIT:
	return ;
}
/* --------------------------------------------------------------------------------------- */
void video_encoder_release()
{
	free(YUV420P_buf);
	free(VOD_encoder.buffer);

	avcodec_close(VOD_encoder.c_context);
	av_free(VOD_encoder.c_context);
	av_free(VOD_encoder.frame_YUV420P);
}
/* --------------------------------------------------------------------------------------- */
//int video_encoder(unsigned char *raw_buf ,unsigned char **ret_buf)
struct AVPacket *video_encoder(unsigned char *raw_buf)
{
	int out_size = 0 ;
	const unsigned char *raw_buf_ptr = raw_buf ;
	int raw_buf_linesize = CAMERA_WIDTH *2 ;

	/* Scale and transform YUV422 format to YUV420P*/

	sws_scale(VOD_encoder.img_convert_ctx,
		&raw_buf_ptr , &raw_buf_linesize,
		0, VOD_encoder.c_context->height,
		VOD_encoder.frame_YUV420P->data, VOD_encoder.frame_YUV420P->linesize);

//	RGB565_to_YUV420P(raw_buf_ptr, VOD_encoder.frame_YUV420P->data[0], CAMERA_WIDTH, CAMERA_HEIGHT);
//	RGB565_to_YUV420P(raw_buf_ptr, VOD_encoder.frame_YUV420P->data[0],VOD_encoder.frame_YUV420P->data[1], VOD_encoder.frame_YUV420P->data[2],  CAMERA_WIDTH, CAMERA_HEIGHT);
//	memcpy(VOD_encoder.frame_YUV420P->data[0], raw_buf ,sizeof(char)*CAMERA_WIDTH* CAMERA_HEIGHT*2 );

	static int pts = 0;
	static int frame_size = 0;
	int got_packet;
	VOD_encoder.frame_YUV420P->pts = pts;
/*
	out_size = avcodec_encode_video(VOD_encoder.c_context,
					VOD_encoder.buffer,
					VOD_encoder.buffer_size,
					VOD_encoder.frame_YUV420P);
*/
	av_init_packet(&VOD_encoder.pkt);
	VOD_encoder.pkt.data = VOD_encoder.buffer;
	VOD_encoder.pkt.size = VOD_encoder.buffer_size;

	out_size = avcodec_encode_video2(VOD_encoder.c_context,
					&VOD_encoder.pkt,
					VOD_encoder.frame_YUV420P,
					&got_packet);

	/* switch to next frame , and return buffer */
	pts++;
	//*ret_buf = VOD_encoder.pkt.data ;
	//return VOD_encoder.pkt.size ;

	return &VOD_encoder.pkt;
}
