
CC := gcc

TARGET := deb-bundle/usr/bin/e502monitor

CFLAGS := -le502api -lx502api -lconfig -pthread -lsndfile -ggdb -g3

SOURCE := src/config.c \
		  src/device.c \
		  src/files.c \
		  src/main.c \
		  src/logging.c \
		  src/pdouble_queue.c \
		  src/frame.c

HEADERS := src/common.h \
		   src/config.h \
		   src/device.h \
		   src/files.h \
		   src/header.h \
		   src/logging.h \
		   src/pdouble_queue.h \
		   src/frame.h

e502monitor: $(SOURCE) $(HEADERS)
	$(CC) $(SOURCE) $(CFLAGS) -o $(TARGET) -DDBG

clean:
	rm deb-bundle/usr/bin/e502monitor