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
#define VIDEO_BUFFER_COUNT	20
static int vbuffer_count = VIDEO_BUFFER_COUNT;
static struct vbuffer buffer[VIDEO_BUFFER_COUNT] ;
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
//	fmt.fmt.pix.pixelformat =V4L2_PIX_FMT_MJPEG;

	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
//	fmt.fmt.pix.field = V4L2_FIELD_ANY;
//	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	if (ioctl(WC, VIDIOC_S_FMT, &fmt) == -1) {
		fprintf(stderr, "Setting Pixel Format");
		return ;
	}

 	/* Request Buffers and Create mmap */
	struct v4l2_requestbuffers req = {0};
	req.count = VIDEO_BUFFER_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(WC, VIDIOC_REQBUFS, &req) == -1) {
		fprintf(stderr, "Requesting Buffer");
		return ;
	}

	printf("request buffer : %d\n",req.count);
	if(req.count != VIDEO_BUFFER_COUNT) {
		vbuffer_count = req.count ;
		printf("Insufficient buffer\n");
	}
	
	/* Query buffer , mmap */
	int buf_ID ;
	for(buf_ID = 0 ; buf_ID < vbuffer_count ; buf_ID++) {
		struct v4l2_buffer buf = {0};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = buf_ID;
		if(ioctl(WC, VIDIOC_QUERYBUF, &buf) == -1) {
			fprintf(stderr, "Querying Buffer");
			return ;
		}
		buffer[buf_ID].length = buf.length;
		buffer[buf_ID].start = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, WC, buf.m.offset);
		printf("mmap Buffer %d\n",buffer[buf_ID].length );
	}
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

	int buf_ID ;
	for(buf_ID = 0 ; buf_ID < vbuffer_count ; buf_ID++) {
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = buf_ID;

		if (ioctl(WC, VIDIOC_QBUF, &buf) ==-1)
			fprintf(stderr, "VIDIOC_QBUF");
	}
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
	int buf_ID ;
	for(buf_ID = 0 ; buf_ID < vbuffer_count ; buf_ID++) {
		if (munmap(buffer[buf_ID].start, buffer[buf_ID].length) == -1)
			fprintf(stderr, "munmap");
	}
}
/* ------------------------------------------------------------ */
struct vbuffer *webcam_read_frame(int WC)
{
	struct v4l2_buffer buf;
	struct vbuffer *cur_webcam =NULL;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(WC, VIDIOC_DQBUF, &buf) == -1) {
		fprintf(stderr, "VIDIOC_DQBUF error\n");
		return 0;
	}
	cur_webcam = &buffer[buf.index];

	if (ioctl(WC, VIDIOC_QBUF, &buf) == -1) {
		fprintf(stderr, "VIDIOC_QBUF error\n");
		return 0;
	}
	return cur_webcam;
}
/* ------------------------------------------------------------ */
int webcam_set_framerate(int WC, int frate)
{
	struct v4l2_streamparm parm;
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(WC, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		printf("Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	printf("Current frame rate: %u/%u\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);

	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = frate;

	ret = ioctl(WC, VIDIOC_S_PARM, &parm);
	if (ret < 0) {
		printf("Unable to set frame rate: %d.\n", errno);
		return ret;
	}

	ret = ioctl(WC, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		printf("Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	printf("Frame rate set: %u/%u\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);
	return 0;
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
				printf("\t%d ,%d \t, ", fsize.discrete.width, fsize.discrete.height);
			} else if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
				printf("\t%d ,%d \t, ", fsize.stepwise.max_width, fsize.stepwise.max_height);
			}

			/* frame rate*/
			struct v4l2_frmivalenum fival;
			memset(&fival, 0, sizeof(fival));
			fival.index = 0;
			fival.pixel_format = fsize.pixel_format;
			fival.width = fsize.discrete.width;
			fival.height = fsize.discrete.height;

			printf("\tTime interval between frame: ");
			while ((ioctl(WC, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0) {
				if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
					printf("%u/%u, ", fival.discrete.numerator, fival.discrete.denominator);
				} else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
					printf("{min { %u/%u } .. max { %u/%u } }, ", fival.stepwise.min.numerator,
						 fival.stepwise.min.numerator,
						fival.stepwise.max.denominator, fival.stepwise.max.denominator);
					break;
	        		} else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
					printf("{min { %u/%u } .. max { %u/%u } / stepsize { %u/%u } }, ",
						fival.stepwise.min.numerator, fival.stepwise.min.denominator,
						fival.stepwise.max.numerator, fival.stepwise.max.denominator,
						fival.stepwise.step.numerator, fival.stepwise.step.denominator);
					break;
				}
				fival.index++;
			}
			printf("\n");
			fsize.index++;
		}
		printf("---------------------------------------------------\n");
	}
}
/* ------------------------------------------------------------ */
