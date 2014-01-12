
all :
	gcc -o main main.c video.c encoder.c -lavformat -lavcodec -lavutil -lswscale

clean :
	rm main
