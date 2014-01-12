#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "video.h"
/* ------------------------------------------------------------ */
#define NET_PORT        5000
#define NET_HOST        "140.125.33.214"
#define NET_BUFFER_SIZE 1028
/* ------------------------------------------------------------ */
struct vbuffer buffer ;
int cam ;
/* ------------------------------------------------------------ */
void init_device()
{
	/* Query the Capture */
	struct v4l2_capability caps = {0};
	if (ioctl(cam, VIDIOC_QUERYCAP, &caps) == -1) {
		perror("Querying Capabilites");
		return ;
	}

	/* Image Format */
	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 320;
	fmt.fmt.pix.height = 240;
	fmt.fmt.pix.pixelformat =V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(cam, VIDIOC_S_FMT, &fmt) == -1) {
		perror("Setting Pixel Format");
		return ;
	}

 	/* Request Buffers */
	struct v4l2_requestbuffers req = {0};
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(cam, VIDIOC_REQBUFS, &req) == -1) {
		perror("Requesting Buffer");
		return ;
	}

	/* Query Buffer */
	struct v4l2_buffer buf = {0};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;
	if(ioctl(cam, VIDIOC_QUERYBUF, &buf) == -1) {
		perror("Querying Buffer");
		return ;
	}
	buffer.length = buf.length;
	buffer.start = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam, buf.m.offset);
	printf("Buffer %d\n",buffer.length );
}
/* ------------------------------------------------------------ */
void open_device()
{
	cam = open("/dev/video0", O_RDWR | O_NONBLOCK, 0);

	if (cam == -1) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", "/dev/video0", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
/* ------------------------------------------------------------ */
void start_capturing()
{
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;

	if (ioctl(cam, VIDIOC_QBUF, &buf) ==-1)
		 perror("VIDIOC_QBUF");

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(cam, VIDIOC_STREAMON, &type) == -1)
		perror("VIDIOC_STREAMON");
}
/* ------------------------------------------------------------ */
void stop_capturing()
{
        enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(cam, VIDIOC_STREAMOFF, &type) == -1)
		perror("VIDIOC_STREAMOFF");
}
/* ------------------------------------------------------------ */
void release_device()
{
	if (munmap(buffer.start, buffer.length) == -1)
		perror("munmap");
}
/* ------------------------------------------------------------ */
int read_frame()
{
	struct v4l2_buffer buf;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(cam, VIDIOC_DQBUF, &buf) == -1) {
		perror("VIDIOC_DQBUF");
		return 0;
	}

	if (ioctl(cam, VIDIOC_QBUF, &buf) == -1) {
		perror("VIDIOC_QBUF");
		return 0;
	}
	return 1;
}
/* ------------------------------------------------------------ */
void show_device_info()
{
	struct v4l2_fmtdesc fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (ioctl(cam, VIDIOC_ENUM_FMT, &fmt) == 0)
	{
		fmt.index++;
		printf("{ pixelformat = %c%c%c%c , description = %s }\n",
			fmt.pixelformat & 0xFF, (fmt.pixelformat >> 8) & 0xFF, (fmt.pixelformat >> 16) & 0xFF,
			(fmt.pixelformat >> 24) & 0xFF, fmt.description);
	}
}
/* ------------------------------------------------------------ */
