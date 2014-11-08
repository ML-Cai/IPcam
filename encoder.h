#ifndef ENCODER_H
#define ENCODER_H
/* ----------------------------------------------------------- */
struct encoder_struct
{
	struct AVCodec *codec;
	struct AVCodecContext *c_context;
	struct AVFrame *frame_YUV420P;
	struct AVPacket pkt; 
	struct SwsContext *img_convert_ctx;

	unsigned char *buffer ;
	int buffer_size;
	unsigned char *picture_buf;
};

/* ----------------------------------------------------------- */
int video_encoder_init(int src_width, int src_height, int src_pixel_fmt, int dst_width, int dst_height);
void video_encoder_release();
//int video_encoder(unsigned char *raw_buf ,unsigned char **ret_buf);
struct AVPacket *video_encoder(unsigned char *raw_buf);
/* ----------------------------------------------------------- */

#endif
