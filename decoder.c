#include <stdio.h>
#include <stdlib.h>

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "decoder.h"

/* ------------------------------------------------------------ */
#define CAMERA_WIDTH	320
#define CAMERA_HEIGHT	240
/* ------------------------------------------------------------ */
struct decoder_struct VOD_decoder ;
/* ------------------------------------------------------------ */
void video_decoder_init()
{
//	VOD_decoder.codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	VOD_decoder.codec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
	if (!VOD_decoder.codec) {
		perror("codec not found\n");
		goto ERROR_EXIT;
	}

	/* alloc Frame buffer */
	int numBytes = avpicture_get_size(PIX_FMT_RGB24, CAMERA_WIDTH, CAMERA_HEIGHT);
	VOD_decoder.buffer = (unsigned char *)av_malloc(sizeof(unsigned char) * numBytes);

	/* alloc FFmpeg resource*/
	VOD_decoder.c_context= avcodec_alloc_context3(VOD_decoder.codec);
	VOD_decoder.frame_RGB24 = avcodec_alloc_frame();
	VOD_decoder.frame_YUV420P = avcodec_alloc_frame();

	avpicture_fill((AVPicture *)VOD_decoder.frame_RGB24, VOD_decoder.buffer, PIX_FMT_RGB24, CAMERA_WIDTH, CAMERA_HEIGHT);	


	/* put sample parameters */
	VOD_decoder.c_context->bit_rate = CAMERA_WIDTH * CAMERA_HEIGHT * 4;

	/* resolution must be a multiple of two */
	VOD_decoder.c_context->width = CAMERA_WIDTH;
	VOD_decoder.c_context->height = CAMERA_HEIGHT;

	/* frames per second */
	VOD_decoder.c_context->time_base.num = 1 ;
	VOD_decoder.c_context->time_base.den = 30;
	VOD_decoder.c_context->gop_size = 10; /* emit one intra frame every ten frames */
	VOD_decoder.c_context->max_b_frames = 0;
	VOD_decoder.c_context->thread_count = 1;
	VOD_decoder.c_context->pix_fmt = PIX_FMT_YUV420P;

	/* open it */
	if (avcodec_open2(VOD_decoder.c_context, VOD_decoder.codec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		goto ERROR_EXIT;
	}


	/* prepare image conveter */
	VOD_decoder.img_convert_ctx = sws_getContext(CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,
						CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_RGB24,
						SWS_POINT, NULL, NULL, NULL);

	if(VOD_decoder.img_convert_ctx == NULL) {
		fprintf(stderr, "Cannot initialize the conversion context!\n");
	}
	printf("Decoder init finish\n");

	return ;

ERROR_EXIT:
	getchar();
	exit(1);
}
/* --------------------------------------------------------------------------------------- */
void video_decoder_release()
{
//	free(VOD_decoder.picture_buf);
	free(VOD_decoder.buffer);

	avcodec_close(VOD_decoder.c_context);
	av_free(VOD_decoder.frame_RGB24);
	av_free(VOD_decoder.frame_YUV420P);
}
/* --------------------------------------------------------------------------------------- */
int video_decoder(unsigned char *raw_buf ,int buf_size, unsigned char **ret_buf)
{
	AVPacket VOD_pkt;
	int delen=0;
	int got_picture =0;


	av_init_packet(&VOD_pkt);
	VOD_pkt.data = raw_buf;
	VOD_pkt.size = buf_size;
	printf("%d\n", VOD_pkt.size);

	if (VOD_pkt.size == 0) {
		fprintf(stderr, "0 size packet\n");
		return 0;
	}
	printf("decode\n");
	delen = avcodec_decode_video2(VOD_decoder.c_context, VOD_decoder.frame_YUV420P, &got_picture, &VOD_pkt);
	if (delen < 0) {
		fprintf(stderr, "Error while decoding frame \n");
		return 0;
	}
	printf("%d\n", delen);
	if (got_picture) {
		printf("got pictur\n");
		/* format to RGB */
		sws_scale(VOD_decoder.img_convert_ctx,
			VOD_decoder.frame_YUV420P->data, VOD_decoder.frame_YUV420P->linesize ,
			0, VOD_decoder.c_context->height,
			VOD_decoder.frame_RGB24->data, VOD_decoder.frame_RGB24->linesize);

		/* feedback data */
		*ret_buf = VOD_decoder.frame_RGB24->data[0];
	}
	return 1;
}
