#ifndef NETWORK_PACKET_H
#define NETWORK_PACKET_H
/* ------------------------------------------------------------ */
#define NET_BUFFER_SIZE 1028
#define NET_DATA_ID_SIZE (sizeof(int))
#define NET_DATA_SLICE_SIZE (NET_BUFFER_SIZE - NET_DATA_ID_SIZE)
/* ------------------------------------------------------------ */
/* thumb ACPacket for network Tx */
struct thumb_AVPacket
{
    int64_t pts;
    int64_t dts;
    int   size;
    int   stream_index;
    int   flags;
    int side_data_elems;
    int   duration;
    int64_t pos;                            ///< byte position in stream, -1 if unknown
    int64_t convergence_duration;
};
/* ------------------------------------------------------------ */
struct StreamPacket_data_slice
{
    int ID ;
    int size ;
    char data[NET_DATA_SLICE_SIZE] ;
} ;
/* ------------------------------------------------------------ */
/* Stream data type */
#define PACKET_TYPE_COMMAND	1
#define PACKET_TYPE_SERVICE	2
#define PACKET_TYPE_CODEC_CONTEXT	4
#define PACKET_TYPE_FRAME_HEADER	8
#define PACKET_TYPE_FRAME_DATA	16
struct StreamPacket_struct
{
    int DataType ;
    union {
        struct thumb_AVPacket header ;
        struct StreamPacket_data_slice data_slice ;
    };
};
/* ------------------------------------------------------------ */
#endif // NETWORK_PACKET_H

