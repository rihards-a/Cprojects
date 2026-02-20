CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lcrypto

TARGET = MD3
SRCS = main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
