#include <stdio.h>
#include <stdlib.h>
#include "video_decoder.h"
/* ------------------------------------------------------------ */
Video_Decoder::Video_Decoder()
{
    CAMERA_WIDTH =0;
    CAMERA_HEIGHT =0;
}
/* ------------------------------------------------------------ */
int Video_Decoder::init(int width, int hegiht, int pixel_fmt)
{
    CAMERA_WIDTH = width ;
    CAMERA_HEIGHT = hegiht ;
//	stream_decoder.codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    stream_decoder.codec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
    if (!stream_decoder.codec) {
        perror("codec not found\n");
        return 0;
    }

    /* alloc Frame buffer */
    int numBytes = avpicture_get_size(PIX_FMT_RGB24, CAMERA_WIDTH, CAMERA_HEIGHT);
    stream_decoder.buffer = (unsigned char *)av_malloc(sizeof(unsigned char) * numBytes);

    /* alloc FFmpeg resource*/
    stream_decoder.c_context= avcodec_alloc_context3(stream_decoder.codec);
    stream_decoder.frame_RGB24 = av_frame_alloc();
    stream_decoder.frame_YUV420P = av_frame_alloc();

    avpicture_fill((AVPicture *)stream_decoder.frame_RGB24, stream_decoder.buffer, PIX_FMT_RGB24, CAMERA_WIDTH, CAMERA_HEIGHT);

    /* put sample parameters */
    stream_decoder.c_context->bit_rate = CAMERA_WIDTH * CAMERA_HEIGHT * 4;

    /* resolution must be a multiple of two */
    stream_decoder.c_context->width = CAMERA_WIDTH;
    stream_decoder.c_context->height = CAMERA_HEIGHT;

    /* frames per second */
    stream_decoder.c_context->time_base.num = 1 ;
    stream_decoder.c_context->time_base.den = 30;
    stream_decoder.c_context->gop_size = 10; /* emit one intra frame every ten frames */
    stream_decoder.c_context->max_b_frames = 0;
    stream_decoder.c_context->thread_count = 1;
    stream_decoder.c_context->pix_fmt = PIX_FMT_YUV420P;

    /* open it */
    if (avcodec_open2(stream_decoder.c_context, stream_decoder.codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        return 0;
    }


    /* prepare image conveter */
    stream_decoder.img_convert_ctx = sws_getContext(CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_YUV420P,
                        CAMERA_WIDTH, CAMERA_HEIGHT, PIX_FMT_RGB24,
                        SWS_POINT, NULL, NULL, NULL);

    if(stream_decoder.img_convert_ctx == NULL) {
        fprintf(stderr, "Cannot initialize the conversion context!\n");
    }
    printf("Decoder init finish\n");

    return 1;
}
/* --------------------------------------------------------------------------------------- */
void Video_Decoder::release()
{
    free(stream_decoder.buffer);
    avcodec_close(stream_decoder.c_context);
    av_free(stream_decoder.frame_RGB24);
    av_free(stream_decoder.frame_YUV420P);
}

/* --------------------------------------------------------------------------------------- */
int Video_Decoder::decode(unsigned char *raw_buf ,int buf_size, unsigned char **ret_buf)
{
    AVPacket stream_pkt;
    int delen=0;
    int got_picture =0;


    av_init_packet(&stream_pkt);
    stream_pkt.data = raw_buf;
    stream_pkt.size = buf_size;

    if (stream_pkt.size == 0) {
        fprintf(stderr, "0 size packet\n");
        return 0;
    }
    delen = avcodec_decode_video2(stream_decoder.c_context, stream_decoder.frame_YUV420P, &got_picture, &stream_pkt);
    if (delen < 0) {
        fprintf(stderr, "Error while decoding frame \n");
        return 0;
    }
    if (got_picture) {
        /* format to RGB */
        sws_scale(stream_decoder.img_convert_ctx,
            stream_decoder.frame_YUV420P->data, stream_decoder.frame_YUV420P->linesize ,
            0, stream_decoder.c_context->height,
            stream_decoder.frame_RGB24->data, stream_decoder.frame_RGB24->linesize);

        /* feedback data */
        *ret_buf = stream_decoder.frame_RGB24->data[0];
    }
    return 1;
}
