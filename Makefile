CC := gcc

TARGET := e502monitor

CFLAGS := -le502api -lx502api -lconfig -pthread

SOURCE := src/config.c \
		  src/device.c \
		  src/files.c \
		  src/main.c \
		  src/pdouble_queue.c

# HEADERS := src/common.h \
# 		   src/config.h \
# 		   src/device.h \
# 		   src/files.h \
# 		   src/header.h \
# 		   src/pdouble_queue.h

e502monitor: $(SOURCE)
	$(CC) $(SOURCE) $(CFLAGS) -o $(TARGET) -DDBG

clean:
	rm e502monitor