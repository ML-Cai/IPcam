#include "mainwindow.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <QApplication>
#include <signal.h>
#include <pthread.h>
#include <linux/videodev2.h>

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
#define __STDC_CONSTANT_MACROS
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
}
#include "video_decoder.h"
#include "glwidget.h"
#include "network_packet.h"

pthread_t Video_Rx_thread ;
/* ------------------------------------------------------------ */
#define VIDEO_NET_PORT	5000
#define AUDIO_NET_PORT	5005

#define STREAM_WIDTH    800
#define STREAM_HEIGHT   600
/* ------------------------------------------------------------ */
static int Rx_socket_init(int *Rx_socket, struct sockaddr_in *Rx_addr, int Rx_port)
{
    /* create socket */
    if((*Rx_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket Failed\n");
        return 0;
    }

    /* resize Buffer size */
    int exBufferSize = 1024 * 1024 * 10 ;
    if(setsockopt(*Rx_socket ,SOL_SOCKET ,SO_RCVBUF , (char*)&exBufferSize  ,sizeof(int)) == -1) {
        perror("setsockopt Failed\n");
        return 0;
    }

    /* Bind */
    Rx_addr->sin_family = AF_INET;
    Rx_addr->sin_port = htons(Rx_port);
    Rx_addr->sin_addr.s_addr = INADDR_ANY;
    if(bind(*Rx_socket, (struct sockaddr *)Rx_addr, sizeof(struct sockaddr_in))== -1) {
        perror("bind error\n");
        return 0;
    }
    return 1;
}
/* -------------------------------------------------------- */
static void *Video_Rx_loop(void * arg)
{
    Video_Decoder Decoder ;
    GLwidget *Viewer = (GLwidget *)arg;
    int Rx_socket;
    struct sockaddr_in Rx_addr;
    struct StreamPacket_struct Rx_Buffer ;
    int recv_len ;
    AVPacket packet ;
    thumb_AVPacket thumb_packet ;
    int remain_size =0;
    unsigned char *RGB_buffer =NULL;	/* decoder feedback buffer pointer */
    int ID ;

    fd_set fds;
    struct timeval tv;
    int r;

    /* Socket init */
    if(Rx_socket_init(&Rx_socket, &Rx_addr, VIDEO_NET_PORT) ==0) {
        perror("Rx Socket init \n");
        return NULL;
    }

    /* Decoder init */
    if(Decoder.init(STREAM_WIDTH, STREAM_HEIGHT, V4L2_PIX_FMT_YUYV) ==0) {
        perror("Decoder init failed\n");
        return NULL;
    }

    QObject::connect(&Decoder, &Video_Decoder::signal_buffering_data ,
                     Viewer, &GLwidget::slots_set_picture);

    packet.data =NULL;
    while(1) {
        /* select and timeout 2s. */
        FD_ZERO(&fds);
        FD_SET(Rx_socket, &fds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(Rx_socket + 1, &fds, NULL, NULL, &tv);
        if (r == -1) {
            perror("select error\n");
            goto VIDEO_RX_RELEASE ;
        }
        if (r == 0) { /* Timeout  */
            continue;
        }

        /* Receive Data */
        recv_len = recv(Rx_socket, (char *)&Rx_Buffer, sizeof(struct StreamPacket_struct), 0) ;
        if(recv_len == -1) {
            perror("Stream data recv() error\n");
            break ;
        }
        else {
            switch(Rx_Buffer.DataType) {
            case PACKET_TYPE_FRAME_HEADER :	/* Stream header info */
                memcpy(&thumb_packet, &Rx_Buffer.header , sizeof(thumb_AVPacket));
                packet.convergence_duration = thumb_packet.convergence_duration;
                packet.dts = thumb_packet.dts;
                packet.duration=thumb_packet.duration;
                packet.flags=thumb_packet.flags;
                packet.pos=thumb_packet.pos;
                packet.pts=thumb_packet.pts;
                packet.side_data_elems=thumb_packet.side_data_elems;
                packet.size=thumb_packet.size;
                packet.stream_index=thumb_packet.stream_index;
                packet.data = (unsigned char *)malloc(sizeof(char) * packet.size);
                remain_size = packet.size;
                break ;
            case PACKET_TYPE_FRAME_DATA :	/* Stream data slice info */
                ID = Rx_Buffer.data_slice.ID ;
                if(packet.data!= NULL) { /* simple bypass stream data before header received */
                    memcpy(packet.data + ID * NET_DATA_SLICE_SIZE, &Rx_Buffer.data_slice.data[0] , Rx_Buffer.data_slice.size);
                    remain_size -= Rx_Buffer.data_slice.size;
                    if(remain_size <= 0) {  /* receive finish */
                        Decoder.decode(packet.data, packet.size, &RGB_buffer); /* Decode frame */
                        emit Decoder.signal_buffering_data(RGB_buffer, STREAM_WIDTH, STREAM_HEIGHT); // Display in Frame buffer
                        free(packet.data);
                        packet.data =NULL;
                    }
                }
                break ;
            }
        }
    }

VIDEO_RX_RELEASE :
    close(Rx_socket);
    printf("Video Rx finish\n");
    pthread_exit(NULL);
}
/* -------------------------------------------------------- */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow window;
    GLwidget * Viewer =NULL;

    window.show();
    av_register_all();

    Viewer = window.get_Viewer();

    pthread_create(&Video_Rx_thread, NULL, Video_Rx_loop, Viewer);

    return a.exec();
}
/* -------------------------------------------------------- */
