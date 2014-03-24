#ifndef VIDEO_H
#define VIDEO_H
/* ------------------------------------------------*/
 struct vbuffer {
	void *start;
	int length;
};

struct webcam_info
{
	int handle;
	int width ;
	int height ;
	int pixel_fmt;
};
/* ------------------------------------------------*/
int webcam_open();
void webcam_init(int width, int height, int WC);
void webcam_release();


void webcam_stop_capturing(int WC);
void webcam_start_capturing(int WC);

int webcam_read_frame(int WC);

void webcam_show_info(int WC);
/* ------------------------------------------------*/
#endif
