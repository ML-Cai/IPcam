#ifndef VOD_NETWORK_PACKET_H
#define VOD_NETWORK_PACKET_H
/* ---------------------------------------------------------------------------------------- */
/* Controller packet
 *
 */
#define VOD_PACKET_TYPE_COMMAND	1
#define VOD_PACKET_TYPE_SERVICE	2
#define VOD_PACKET_TYPE_CODEC_CONTEXT	4
#define VOD_PACKET_TYPE_FRAME_HEADER	8
#define VOD_PACKET_TYPE_FRAME_DATA	16

/* Code Contex packet */
struct VOD_packet_CodecContext_slice
{
	int bit_rate;
	int width;
	int height;
	int time_base_den;
	int time_base_num;
	int gop_size;
	int max_b_frames;
	enum AVPixelFormat pix_fmt;
	enum AVCodecID codec_id;
	enum AVMediaType codec_type ;
	int extradata_size ;
};
struct VOD_packet_CodecContext
{
	struct VOD_packet_CodecContext_slice data;
	char extradata[256];
};
/* -------------------------------------------------------- */
enum VOD_command {
	COMMAND_STOP		= 1,
	COMMAND_PLAY		= 2,
	COMMAND_SEEK		= 4,
	COMMAND_FORWARD		= 8,
	COMMAND_BACKWARD	= 16,
};

enum VOD_service{
	REQ_LIST 		= 1,
	REQ_CODEC 		= 2,
	REQ_SEGMENT 		= 4,
	REQ_LAST_FRAME_ID	= 8,
};

/* VOD stream command */
struct VOD_packet_command 
{
	enum VOD_command controller ;
	char Data[256] ;
} ;

/* VOD stream service request */
struct VOD_packet_service
{
	enum VOD_service controller ;
	char Data[256] ;
} ;
/* -------------------------------------------------------- */
/* VOD controller packet */
struct VOD_ControllerPacket_struct
{
	int ControlType ;
	union
	{
		struct VOD_packet_command command ;
		struct VOD_packet_service service ;
		struct VOD_packet_CodecContext codec_contex ;
	};
};
/* ---------------------------------------------------------------------------------------- */
/* Data packet
 *
 */
/* VOD AVPacket data slice , per slice 1024 bit ,and it's rebuild ID*/
struct VOD_packet_frame_data_slice
{
	int ID ;
	int size ;
	char data[1024] ;
} ;
//#pragma pack(1)
struct VOD_DataPacket_struct
{
	int DataType ;
	union
	{
		struct AVPacket header ;
		struct VOD_packet_frame_data_slice data ;
	};
};
//#pragma pack()
/* -------------------------------------------------------- */
#endif
