#include <stdio.h>
#include "format.h"
/* -------------------------------------------------- */
void RGB24_to_RGB565(unsigned char *RGB24_ptr, unsigned char *RGB565_ptr, int width, int height)
{
	int x, y ;
	unsigned short *RGB565_save_ptr = (unsigned short *)RGB565_ptr;
	int index_24 ;
	int index_16 ;
	int R, G ,B ;

	for(y =	0 ; y < height ; y++) {
		for(x = 0 ; x < width ; x++) {
			index_24 = (y * width + x) *3 ;
			index_16 = (y * width + x) *2 ;
			RGB565_save_ptr = (short *)(RGB565_ptr + index_16) ;
			R = *(RGB24_ptr + index_24);
			G = *(RGB24_ptr + index_24+1);
			B = *(RGB24_ptr + index_24+2);

			*RGB565_save_ptr = ((R >>3) <<11) | ((G>>2) << 5) | (B>>3);
		}
	}
}














