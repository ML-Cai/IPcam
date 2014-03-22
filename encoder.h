#ifndef ENCODER_H
#define ENCODER_H
/* ----------------------------------------------------------- */
struct M_encoder_struct
{
	struct AVCodec *codec;
	struct AVCodecContext *c_context;
	struct AVFrame *picture_src;
	struct AVFrame *picture_dst;
	struct SwsContext *img_convert_ctx;

	unsigned char *buffer ;
	int outbuf_size;
	unsigned char *picture_buf;;
};
/* ----------------------------------------------------------- */
#endif
