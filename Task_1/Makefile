CC = g++
CFLAGS = -O2 -Wall

ifdef USE_DOUBLE
    CFLAGS += -DUSE_DOUBLE=1
else
    CFLAGS += -DUSE_DOUBLE=0
endif

all: sin_array_sum

sin_array_sum: src/main.cpp
	$(CC) $(CFLAGS) src/main.cpp -o sin_array_sum

clean:
	rm -f sin_array_sum