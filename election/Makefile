CC = gcc
CFLAGS = -Wall -Wextra -lrt
TARGET = a.out
SRCS = *.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET)