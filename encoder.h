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
	unsigned char *picture_buf;;
};
/* ----------------------------------------------------------- */
void video_encoder_init(int width, int hegiht, int pixel_fmt);
void video_encoder_release();
//int video_encoder(unsigned char *raw_buf ,unsigned char **ret_buf);
struct AVPacket *video_encoder(unsigned char *raw_buf);
/* ----------------------------------------------------------- */

#endif
