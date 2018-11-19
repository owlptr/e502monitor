CC := gcc

TARGET := e502monitor

CFLAGS := -le502api -lx502api -lconfig -pthread
CFLAGS += -Iinclude/ -Llib/


e502monitor: e502monitor.c 
	$(CC) e502monitor.c pdouble_queue.c $(CFLAGS) -o $(TARGET)

clean:
	rm e502monitor