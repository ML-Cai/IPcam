#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H
/* ----------------------------------------------------------- */
#include <QObject>
#define __STDC_CONSTANT_MACROS
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/rational.h>
}
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
/* ----------------------------------------------------------- */
class Video_Decoder : public QObject
{
    Q_OBJECT
public:
    Video_Decoder();
    int init(int width, int hegiht, int pixel_fmt);
    void release();
    int decode(unsigned char *raw_buf ,int buf_size, unsigned char **ret_buf);
private :
    int CAMERA_WIDTH ;
    int CAMERA_HEIGHT;
    struct decoder_struct stream_decoder ;
signals :
    void signal_buffering_data(unsigned char *color, int SizeWidth, int SizeHeight);
};
/* ----------------------------------------------------------- */
#endif // VIDEO_DECODER_H
