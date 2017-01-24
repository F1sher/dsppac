CC=g++
CFLAGS=-g -Wall -O0
LFLAGS=-L ./lib/ 
LIBS=-l cyusb -lm
DEPS=../dsp.h ../dsp.c
SRCS=../dsp.c

#UNITY for unit tests
UNITY_SRC=../unity/unity.c

main_test_1: $(DEPS) ../main_test_1.c $(UNITY_SRC)
	$(CC) -o $@ ../main_test_1.c $(UNITY_SRC) $(SRCS) $(LFLAGS) $(LIBS) $(CFLAGS)

drainer: $(DEPS) ../drainer.c
	$(CC) -o $@ ../drainer.c $(SRCS) $(LFLAGS) $(LIBS) $(CFLAGS)

clean:
	rm ./*.o
