#include <stdio.h>
#include <stdlib.h>

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "encoder.h"

/* ------------------------------------------------------------ */
#define CAMERA_WIDTH	320
#define CAMERA_HEIGHT	240
/* ------------------------------------------------------------ */
int cam ;
struct M_encoder_struct M_encoder ;
unsigned char YUV420P_buf[CAMERA_HEIGHT][CAMERA_WIDTH][2] ={0};
/* ------------------------------------------------------------ */
void video_encoder_init()
{
//	M_encoder.codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	M_encoder.codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!M_encoder.codec) {
		perror("codec not found\n");
		goto ERROR_EXIT;
	}

	M_encoder.c_context= avcodec_alloc_context3(M_encoder.codec);
	M_encoder.picture_src= av_frame_alloc();

	/* put sample parameters */
	M_encoder.c_context->bit_rate = CAMERA_WIDTH * CAMERA_HEIGHT * 4;

	/* resolution must be a multiple of two */
	M_encoder.c_context->width = CAMERA_WIDTH;
	M_encoder.c_context->height = CAMERA_HEIGHT;

	/* frames per second */
	M_encoder.c_context->time_base.num = 1 ;
	M_encoder.c_context->time_base.den = 30;
	M_encoder.c_context->gop_size = 10; /* emit one intra frame every ten frames */
	M_encoder.c_context->max_b_frames = 0;
	M_encoder.c_context->thread_count = 1;
	M_encoder.c_context->pix_fmt = PIX_FMT_YUV420P;

	/* open it */
	if (avcodec_open2(M_encoder.c_context, M_encoder.codec, NULL) < 0) {
		perror("could not open codec\n");
		goto ERROR_EXIT;
	}

	/* prepare Codec buffer */
	M_encoder.outbuf_size = CAMERA_WIDTH*CAMERA_HEIGHT *2;
	M_encoder.buffer = (unsigned char *)malloc(M_encoder.outbuf_size);
	memset(M_encoder.buffer, 0 ,M_encoder.outbuf_size);

	M_encoder.picture_buf = &YUV420P_buf[0][0][0];
	M_encoder.picture_src->data[0] = M_encoder.picture_buf;
	M_encoder.picture_src->data[1] = M_encoder.picture_src->data[0] + (CAMERA_WIDTH * CAMERA_HEIGHT);
	M_encoder.picture_src->data[2] = M_encoder.picture_src->data[1] + (CAMERA_WIDTH * CAMERA_HEIGHT) / 4;

	M_encoder.picture_src->linesize[0] = M_encoder.c_context->width;
	M_encoder.picture_src->linesize[1] = M_encoder.c_context->width / 2;
	M_encoder.picture_src->linesize[2] = M_encoder.c_context->width / 2;

	/* prepare image conveter */
/*
	M_encoder.img_convert_ctx = sws_getContext( CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_NV12, //PIX_FMT_YUYV422,
                                                CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
                                                SWS_POINT, NULL, NULL, NULL);

*/


/*
        M_encoder.img_convert_ctx = sws_getContext( CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P, //PIX_FMT_YUYV422,
                                                CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
                                                SWS_POINT, NULL, NULL, NULL);
*/


	M_encoder.img_convert_ctx = sws_getContext( CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUYV422, //PIX_FMT_YUYV422,
						CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
						SWS_POINT, NULL, NULL, NULL);


    if(M_encoder.img_convert_ctx == NULL) {
        perror("Cannot initialize the conversion context!\n");
        goto ERROR_EXIT;
    }
	printf("encoder init finish\n");

	return ;

ERROR_EXIT:
	getchar();
	exit(1);
}
/* --------------------------------------------------------------------------------------- */
void video_encoder_release()
{
//	free(M_encoder.picture_buf);
	free(M_encoder.buffer);

	avcodec_close(M_encoder.c_context);
	av_free(M_encoder.c_context);
	av_free(M_encoder.picture_src);
//	av_free(M_encoder.picture_dst);
}
/* --------------------------------------------------------------------------------------- */
int video_encoder(unsigned char *raw_buf ,unsigned char **ret_buf)
{
	int out_size = 0 ;
	const unsigned char *raw_buf_ptr = raw_buf ;
	int raw_buf_linesize = CAMERA_WIDTH *2 ;


	/* Scale and transform YUV422 format to YUV420P*/

	sws_scale(M_encoder.img_convert_ctx,
		&raw_buf_ptr , &raw_buf_linesize,
		0, M_encoder.c_context->height,
		M_encoder.picture_src->data, M_encoder.picture_src->linesize);

//	memcpy(M_encoder.picture_src->data[0], raw_buf ,sizeof(char)*CAMERA_WIDTH* CAMERA_HEIGHT*2 );

	static int pts = 0;
	static int frame_size = 0;
	M_encoder.picture_src->pts = pts;
	out_size = avcodec_encode_video(M_encoder.c_context,
					M_encoder.buffer,
					M_encoder.outbuf_size,
					M_encoder.picture_src);
	/* switch to next frame , and return buffer */
	pts++;
	*ret_buf = M_encoder.buffer ;

	return out_size ;
}
