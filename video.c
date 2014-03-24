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
struct vbuffer buffer ;
/* ------------------------------------------------------------ */
void webcam_init(int width, int height, int WC)
{
	/* Query the Capture */
	struct v4l2_capability caps = {0};
	if (ioctl(WC, VIDIOC_QUERYCAP, &caps) == -1) {
		fprintf(stderr, "Querying Capabilites");
		return ;
	}

	/* Image Format */
	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat =V4L2_PIX_FMT_YUYV;
//	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(WC, VIDIOC_S_FMT, &fmt) == -1) {
		fprintf(stderr, "Setting Pixel Format");
		return ;
	}

 	/* Request Buffers and Create mmap */
	struct v4l2_requestbuffers req = {0};
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(WC, VIDIOC_REQBUFS, &req) == -1) {
		fprintf(stderr, "Requesting Buffer");
		return ;
	}

	/* Query Buffer */
	struct v4l2_buffer buf = {0};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;
	if(ioctl(WC, VIDIOC_QUERYBUF, &buf) == -1) {
		fprintf(stderr, "Querying Buffer");
		return ;
	}
	buffer.length = buf.length;
	buffer.start = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, WC, buf.m.offset);
	printf("Buffer %d\n",buffer.length );
}
/* ------------------------------------------------------------ */
int webcam_open()
{
	int cam = open("/dev/video0", O_RDWR | O_NONBLOCK, 0);

	if (cam == -1) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", "/dev/video0", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	/* return device handle */
	return cam;
}
/* ------------------------------------------------------------ */
void webcam_start_capturing(int WC)
{
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;

	if (ioctl(WC, VIDIOC_QBUF, &buf) ==-1)
		 fprintf(stderr, "VIDIOC_QBUF");

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(WC, VIDIOC_STREAMON, &type) == -1)
		fprintf(stderr, "VIDIOC_STREAMON");
}
/* ------------------------------------------------------------ */
void webcam_stop_capturing(int WC)
{
        enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(WC, VIDIOC_STREAMOFF, &type) == -1)
		fprintf(stderr, "VIDIOC_STREAMOFF");
}
/* ------------------------------------------------------------ */
void webcam_release()
{
	if (munmap(buffer.start, buffer.length) == -1)
		fprintf(stderr, "munmap");
}
/* ------------------------------------------------------------ */
int webcam_read_frame(int WC)
{
	struct v4l2_buffer buf;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(WC, VIDIOC_DQBUF, &buf) == -1) {
		fprintf(stderr, "VIDIOC_DQBUF");
		return 0;
	}

	if (ioctl(WC, VIDIOC_QBUF, &buf) == -1) {
		fprintf(stderr, "VIDIOC_QBUF");
		return 0;
	}
	return 1;
}
/* ------------------------------------------------------------ */
void webcam_show_info(int WC)
{
	struct v4l2_fmtdesc fmt;
	struct v4l2_frmsizeenum fsize;

	memset(&fmt, 0, sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (ioctl(WC, VIDIOC_ENUM_FMT, &fmt) == 0) {
		/* pixel format */
		printf("{ pixelformat = %c%c%c%c , description = %s }\n",
			fmt.pixelformat & 0xFF, (fmt.pixelformat >> 8) & 0xFF, (fmt.pixelformat >> 16) & 0xFF,
			(fmt.pixelformat >> 24) & 0xFF, fmt.description);
		fmt.index++;

		/* pixel size */
		fsize.pixel_format = fmt.pixelformat;
		fsize.index = 0;
		while (ioctl(WC, VIDIOC_ENUM_FRAMESIZES, &fsize) >= 0) {
			if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				printf("%d ,%d\n", fsize.discrete.width, fsize.discrete.height);
			} else if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
				printf("%d ,%d\n", fsize.stepwise.max_width, fsize.stepwise.max_height);
			}
			fsize.index++;
		}
	}
}
/* ------------------------------------------------------------ */
