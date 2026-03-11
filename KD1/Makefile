CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = kd1
SRCS = kd1.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
