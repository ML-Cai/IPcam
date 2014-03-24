#ifndef DECODER_H 
#define DECODER_H
/* ----------------------------------------------------------- */
struct decoder_struct 
{
	struct AVCodec *codec;
	struct AVCodecContext *c_context;
	struct AVFrame *frame_RGB24;
	struct AVFrame *frame_YUV420P;
	struct SwsContext *img_convert_ctx;

	unsigned char *buffer ;
	int outbuf_size;
};

void video_decoder_init(int width, int hegiht, int pixel_fmt);
void video_decoder_release();
int video_decoder(unsigned char *raw_buf ,int buf_size, unsigned char **ret_buf);
/* ----------------------------------------------------------- */
#endif
