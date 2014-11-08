#include <stdio.h>
#include <stdlib.h>
#include <linux/videodev2.h>

#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>

#include "encoder.h"
/* ------------------------------------------------------------ */
static int SRC_WIDTH = 0;
static int SRC_HEIGHT = 0;
static int DST_WIDTH = 0;
static int DST_HEIGHT = 0;
/* ------------------------------------------------------------ */
static struct encoder_struct VOD_encoder ;
static unsigned char *YUV420P_buf =NULL ;
/* ------------------------------------------------------------ 
 * encoder_struct initial function */
static void init_decoder_sturct(struct encoder_struct * src)
{	
	src->codec = NULL;
	src->c_context = NULL;
	src->frame_YUV420P = NULL;
	av_init_packet(&src->pkt);
	src->img_convert_ctx = NULL;
	src->buffer = NULL;
	src->buffer_size = 0 ;
	src->picture_buf = NULL;
}

/* ------------------------------------------------------------ 
 * Video Encoder init 
 *		Initial this function first befoer using any function in video_endocder.c .
 *
 *		@param src_width : soruce image data width
 *		@param src_height : soruce image data height
 *		@param src_pixel_fmt : sorce image pixel format , YUV422 / YUV420 .....
 *		@param dst_width : encode frame width
 *		@param dst_width : encode frame height
 *
 *		@return : 1 for successed , 0 for failed .
 */
int video_encoder_init(int src_width, int src_height, int src_pixel_fmt, int dst_width, int dst_height)
{
	SRC_WIDTH = src_width;
	SRC_HEIGHT = src_height;
	DST_WIDTH = dst_width;
	DST_HEIGHT = dst_height;
	init_decoder_sturct(&VOD_encoder);

//	VOD_encoder.codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	VOD_encoder.codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
//	VOD_encoder.codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);

	if (!VOD_encoder.codec) {
		perror("codec not found\n");
		goto ERROR_EXIT;
	}
	VOD_encoder.c_context= avcodec_alloc_context3(VOD_encoder.codec);
	VOD_encoder.frame_YUV420P= av_frame_alloc();

	/* put sample parameters */
	VOD_encoder.c_context->bit_rate = DST_WIDTH * DST_HEIGHT * 4;

	/* resolution must be a multiple of two */
	VOD_encoder.c_context->width = DST_WIDTH;
	VOD_encoder.c_context->height = DST_HEIGHT;
	printf("encoder %d %d\n",VOD_encoder.c_context->width ,VOD_encoder.c_context->height);

	/* frames per second */
	VOD_encoder.c_context->time_base.num = 1 ;
	VOD_encoder.c_context->time_base.den = 30;
	VOD_encoder.c_context->gop_size = 60;
	VOD_encoder.c_context->max_b_frames = 0;
	VOD_encoder.c_context->thread_count = 4;
	VOD_encoder.c_context->pix_fmt = PIX_FMT_YUV420P;

	/* open codec */
	if (avcodec_open2(VOD_encoder.c_context, VOD_encoder.codec, NULL) < 0) {
		perror("could not open codec\n");
		goto ERROR_EXIT;
	}

	/* prepare Codec buffer */
	VOD_encoder.buffer_size = DST_WIDTH * DST_HEIGHT *3;	/* multiply 3 for RGB */
	VOD_encoder.buffer = (unsigned char *)malloc(VOD_encoder.buffer_size);
	av_init_packet(&VOD_encoder.pkt);
	VOD_encoder.pkt.data = VOD_encoder.buffer;
	VOD_encoder.pkt.size = VOD_encoder.buffer_size;


	YUV420P_buf = (unsigned char *)malloc(DST_HEIGHT * DST_WIDTH * 2);	/* multiply 2 for YUV420P */
	memset(YUV420P_buf, 0, DST_HEIGHT * DST_WIDTH * 2);
	VOD_encoder.picture_buf = YUV420P_buf;
	VOD_encoder.frame_YUV420P->data[0] = VOD_encoder.picture_buf;
	VOD_encoder.frame_YUV420P->data[1] = VOD_encoder.frame_YUV420P->data[0] + (DST_WIDTH * DST_HEIGHT);
	VOD_encoder.frame_YUV420P->data[2] = VOD_encoder.frame_YUV420P->data[1] + (DST_WIDTH * DST_HEIGHT) / 4;

	VOD_encoder.frame_YUV420P->linesize[0] = VOD_encoder.c_context->width;
	VOD_encoder.frame_YUV420P->linesize[1] = VOD_encoder.c_context->width / 2;
	VOD_encoder.frame_YUV420P->linesize[2] = VOD_encoder.c_context->width / 2;

	/* prepare image conveter
	 *		Conveter Source data format into specify data format .
	 */
	switch(src_pixel_fmt) {
	case V4L2_PIX_FMT_YUV420 :
		printf("<< Source PIX_FMT_YUV420 >>\n");	
		VOD_encoder.img_convert_ctx = sws_getContext(SRC_WIDTH, SRC_HEIGHT, PIX_FMT_YUV420P, //PIX_FMT_YUYV422,
                        	                        DST_WIDTH, DST_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
                	                                SWS_POINT, NULL, NULL, NULL);
		break ;
	case V4L2_PIX_FMT_YUYV :
		printf("<< Source PIX_FMT_YUYV >>\n");	
		VOD_encoder.img_convert_ctx = sws_getContext(SRC_WIDTH, SRC_HEIGHT, PIX_FMT_YUYV422, //PIX_FMT_YUYV422,
													DST_WIDTH, DST_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
													SWS_POINT, NULL, NULL, NULL);
		break ;
	case V4L2_PIX_FMT_MJPEG :
		printf("<< Source PIX_FMT_MJPEG >>\n");	
		VOD_encoder.img_convert_ctx = sws_getContext(SRC_WIDTH, SRC_HEIGHT, AV_PIX_FMT_YUVJ422P,
													DST_WIDTH, DST_HEIGHT, PIX_FMT_YUV420P,
													SWS_POINT, NULL, NULL, NULL);
		break;
	case PIX_FMT_RGB565LE :
		printf("<< PIX_FMT_RGB565LE >>\n"); 
		VOD_encoder.img_convert_ctx = sws_getContext(SRC_WIDTH, SRC_HEIGHT, PIX_FMT_RGB565LE, //PIX_FMT_YUYV422,
                        	                        DST_WIDTH, DST_HEIGHT, PIX_FMT_YUV420P,  //PIX_FMT_YUV420P,
													SWS_POINT, NULL, NULL, NULL);
		break;
	}	
	if(VOD_encoder.img_convert_ctx == NULL) {
		perror("Cannot initialize the conversion context!\n");
		goto ERROR_EXIT;
	}

	printf("Encoder init finish\n");

	return 1;

ERROR_EXIT:
	return 0;
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
/* --------------------------------------------------------------------------------------- 
 * Video Encoder
 *		Encode source data into specify stream format (H.264 / MPEG 4) . This function endcode source_buf frame with previously data .
 *		the return value is the I/P-frame data after encode .
 *
 *		@param source_buf : source data from webcam or other devie/memory , source data format was specyfied in video_encoder_init() function.
 *
 *		@return : return AVPacket * for source_buf .
 */
struct AVPacket *video_encoder(unsigned char *source_buf)
{
	int err ;
	const unsigned char *raw_buf_ptr = source_buf ;
	int source_buf_linesize = SRC_WIDTH *2 ;
	static int pts = 0;
	static int frame_size = 0;
	int got_packet;
	VOD_encoder.frame_YUV420P->pts = pts;

	/* Scale and transform YUV422 format to YUV420P*/
	sws_scale(VOD_encoder.img_convert_ctx, &raw_buf_ptr, &source_buf_linesize, 0, VOD_encoder.c_context->height,
				VOD_encoder.frame_YUV420P->data, VOD_encoder.frame_YUV420P->linesize);

	/* Buffering data */
	av_init_packet(&VOD_encoder.pkt);
	VOD_encoder.pkt.data = VOD_encoder.buffer;
	VOD_encoder.pkt.size = VOD_encoder.buffer_size;

	/* Encode */
	err = avcodec_encode_video2(VOD_encoder.c_context, &VOD_encoder.pkt, VOD_encoder.frame_YUV420P, &got_packet);
	if(err !=0) {
		fprintf(stderr, "avcodec_encode_video2 failed , code : %d (%X) \n", err, err);
	}
	else {
		/* switch to next frame */
		pts++;
	}

	/* return frame packet */
	return &VOD_encoder.pkt;
}
