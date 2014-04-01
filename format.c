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


inline void RGB565_2RGB(int sourceRGB, int *R, int *G, int *B)
{
	*R = (sourceRGB >> 11) << 3 ;
	*G = ((sourceRGB & 0x7e0 ) >>5) <<2;
	*B = ((sourceRGB& 0x1f)) << 3 ;
}
inline void RGB565_2_YUV(int sourceRGB565, unsigned int *Y, unsigned int *U, unsigned int *V)
{
	int R, G, B;
	int y, u, v;
	R = (sourceRGB565 >> 11) << 3 ;
	G = ((sourceRGB565 & 0x7e0 ) >>5) <<2;
	B = ((sourceRGB565 & 0x1f)) << 3 ;
	y = (306 *R + 601 *G + 116 *B) >> 10;
//	u = (-150 *R - 295 *G + 446 *B) >> 10;
//	v = (629 *R - 527 *G - 102 *B) >> 10 ;
	u = ((577 * (B - y)) >>10)+128 ;
	v = ((730 * (R - y)) >>10)+128 ;

	if(u>255) u = 255;
	if(v>255) v = 255;
	if(u <0 ) u = 0;
	if(v <0 ) v = 0;
	*Y |= y;
	*U |= u;
	*V |= v;
}

inline void RGB565_2_Y(int sourceRGB565, unsigned int *Y)
{
	int R, G, B;
	R = (sourceRGB565 >> 11) << 3 ;
	G = ((sourceRGB565 & 0x7e0 ) >>5) <<2;
	B = ((sourceRGB565 & 0x1f)) << 3 ;
	*Y |= (306 *R + 601 *G + 116 *B) >> 10;
	
}


/* -------------------------------------------------- */
//void RGB565_to_YUV420P(unsigned char *source_RGB565_ptr, unsigned char *source_YUV420P_ptr, int width, int height)
void RGB565_to_YUV420P(unsigned char *source_RGB565_ptr, unsigned char *source_Y_ptr,unsigned char *source_U_ptr,unsigned char *source_V_ptr, int width, int height)

{
	int x, y ;
	int pre_index , index ;
	int R, G, B ;
	int Y, U, V ;
//	unsigned int *Y_ptr = (unsigned int *)source_YUV420P_ptr;
//	unsigned int *U_ptr = (unsigned int *)(source_YUV420P_ptr + (width * height)) ;
//	unsigned int *V_ptr = (unsigned int *)(source_YUV420P_ptr + (width * height) + (width * height) /4);
	unsigned int *Y_ptr = (unsigned int *)source_Y_ptr;
	unsigned int *U_ptr = (unsigned int *)source_U_ptr;
	unsigned int *V_ptr = (unsigned int *)source_V_ptr;


	unsigned int *RGB565_ptr = (unsigned int *)source_RGB565_ptr;
	unsigned int RGB565_Data_12;
	unsigned int RGB565_Data_34;
	unsigned int RGB565_Data_56;
	unsigned int RGB565_Data_78;
	unsigned int RGB565_Data_1;
	unsigned int RGB565_Data_2;
	unsigned int RGB565_Data_3;
	unsigned int RGB565_Data_4;
	unsigned int RGB565_Data_5;
	unsigned int RGB565_Data_6;
	unsigned int RGB565_Data_7;
	unsigned int RGB565_Data_8;
	
	int U_count =0;

	width /=8;
	for(y = 0 ; y < height ; y++) {
		for(x = 0 ; x < width ; x++) {
			Y =0;
			U =0;
			V =0;
			RGB565_Data_12 = *RGB565_ptr++;
			RGB565_Data_34 = *RGB565_ptr++;
			RGB565_Data_56 = *RGB565_ptr++;
			RGB565_Data_78 = *RGB565_ptr++;

			RGB565_Data_1 = RGB565_Data_12 &0x0000FFFF ;
			RGB565_Data_2 = RGB565_Data_12 >> 16;
			RGB565_Data_3 = RGB565_Data_34 &0x0000FFFF ;
			RGB565_Data_4 = RGB565_Data_34 >> 16;
			RGB565_Data_5 = RGB565_Data_56 &0x0000FFFF ;
			RGB565_Data_6 = RGB565_Data_56 >> 16;
			RGB565_Data_7 = RGB565_Data_78 &0x0000FFFF ;
			RGB565_Data_8 = RGB565_Data_78 >> 16;

			RGB565_2_YUV(RGB565_Data_4, &Y, &U, &V);
			Y <<=8;
			U <<=8;
			V <<=8;

			RGB565_2_YUV(RGB565_Data_3, &Y, &U, &V);
			Y <<=8;


			RGB565_2_YUV(RGB565_Data_2, &Y, &U, &V);
			Y <<=8;
			U <<=8;
			V <<=8;

			RGB565_2_YUV(RGB565_Data_1, &Y, &U, &V);
			*Y_ptr++ = Y;
			Y=0;

			/* -------------------------------------- */
			RGB565_2_YUV(RGB565_Data_8, &Y, &U, &V);
			Y <<=8;
			U <<=8;
			V <<=8;

			RGB565_2_YUV(RGB565_Data_7, &Y, &U, &V);
			Y <<=8;


			RGB565_2_YUV(RGB565_Data_6, &Y, &U, &V);
			Y <<=8;

			RGB565_2_YUV(RGB565_Data_5, &Y, &U, &V);
			*Y_ptr++ = Y;
			*U_ptr++ = U;
			*V_ptr++ = V;
//			*U_ptr++ = 255;
//			*V_ptr++ = 255;
		}
		y++;
		for(x = 0 ; x < width ; x++) {
			Y =0;
			U =0;
			V =0;
			RGB565_Data_12 = *RGB565_ptr++;
			RGB565_Data_34 = *RGB565_ptr++;
			RGB565_Data_56 = *RGB565_ptr++;
			RGB565_Data_78 = *RGB565_ptr++;

			RGB565_Data_1 = RGB565_Data_12 &0x0000FFFF ;
			RGB565_Data_2 = RGB565_Data_12 >> 16;
			RGB565_Data_3 = RGB565_Data_34 &0x0000FFFF ;
			RGB565_Data_4 = RGB565_Data_34 >> 16;
			RGB565_Data_5 = RGB565_Data_56 &0x0000FFFF ;
			RGB565_Data_6 = RGB565_Data_56 >> 16;
			RGB565_Data_7 = RGB565_Data_78 &0x0000FFFF ;
			RGB565_Data_8 = RGB565_Data_78 >> 16;

			RGB565_2_Y(RGB565_Data_4, &Y);
			Y <<=8;

			RGB565_2_Y(RGB565_Data_3, &Y);
			Y <<=8;

			RGB565_2_Y(RGB565_Data_2, &Y);
			Y <<=8;
//
			RGB565_2_Y(RGB565_Data_1, &Y);
			*Y_ptr++ = Y;
			Y=0;

			/* -------------------------------------- */

			RGB565_2_Y(RGB565_Data_8, &Y);
			Y <<=8;

			RGB565_2_Y(RGB565_Data_7, &Y);
			Y <<=8;

			RGB565_2_Y(RGB565_Data_6, &Y);
			Y <<=8;

			RGB565_2_Y(RGB565_Data_5, &Y);
			*Y_ptr++ = Y;
		}
	}
}









