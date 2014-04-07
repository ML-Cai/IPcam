LIB_PATH = ../BBBIOlib/BBBio_lib/

all : server client

server : server.c video.o encoder.o decoder.o format.o
	gcc -o server server.c video.o encoder.o decoder.o format.o -lavformat -lavcodec -lavutil -lswscale -pthread -O3 -L ${LIB_PATH} -lBBBio

client : client.c video.o encoder.o decoder.o format.o
	gcc -o client client.c video.o encoder.o decoder.o format.o -lavformat -lavcodec -lavutil -lswscale -pthread


video.o : video.c video.h
	gcc -c video.c

encoder.o : encoder.c encoder.h
	gcc -c encoder.c

decoder.o : decoder.c decoder.h
	gcc -c decoder.c

format.o : format.c format.h
	gcc -c format.c -O3


clean :
	rm client server video.o encoder.o decoder.o format.o
