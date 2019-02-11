CC=g++
CFLAGS=-g -Wall -O0
LFLAGS=-L ./lib/
IFLAGS=-I/usr/local/include -I./sclog4c/include
LIBS=-lcyusb -lusb-1.0 -lm -ljsmn
DEPS=../dsp.h ../dsp.c ../parse_args.h ../parse_args.c
SRCS=../dsp.c ../parse_args.c

#UNITY for unit tests
UNITY_SRC=../unity/unity.c

hdrainer: $(DEPS) ../hdrainer.c send_comm
	$(CC) -o $@ ../hdrainer.c $(SRCS) ./sclog4c/src/libsclog4c.a $(LFLAGS) $(IFLAGS) $(LIBS) $(CFLAGS) -lzmq -lgsl -lgslcblas -DDEBUG 

oconsumer: $(DEPS) ../oconsumer.c
	$(CC) -o $@ ../oconsumer.c $(SRCS) $(LFLAGS) $(LIBS) $(CFLAGS)

send_comm: $(DEPS) ../send_comm.c
	$(CC) -o $@ ../send_comm.c $(SRCS) $(LFLAGS) $(IFLAGS) $(LIBS) $(CFLAGS) -lgsl -lgslcblas

usb_speedtest: $(DEPS) ../usb_speedtest.c
	$(CC) -o $@ ../usb_speedtest.c $(SRCS) $(LFLAGS) $(IFLAGS) $(LIBS) $(CFLAGS) -lgsl -lgslcblas -DDEBUG

clean:
	rm ./*.o
