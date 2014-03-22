
all :
	gcc -o main main.c video.c encoder.c decoder.c format.c -lavformat -lavcodec -lavutil -lswscale

clean :
	rm main
