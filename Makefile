FFMPEG_PATH = /opt/FFmpeg

FFMPEG_INCLUDE_PATH = ${FFMPEG_PATH}/include
FFMPEG_LIB_PATH = ${FFMPEG_PATH}/lib
FFMPEG_LIB_SET = -lavutil -lavcodec -lavformat -lswscale -lswresample

all : IPcam

IPcam : main.c video.o encoder.o
	gcc -o IPcam main.c video.o encoder.o -I${FFMPEG_PATH}/include -L${FFMPEG_PATH}/lib ${FFMPEG_LIB_SET} -pthread -O3

video.o : video.c video.h
	gcc -c video.c -I${FFMPEG_INCLUDE_PATH}

encoder.o : encoder.c encoder.h
	gcc -c encoder.c -I${FFMPEG_INCLUDE_PATH}

clean :
	rm -f IPcam video.o encoder.o
