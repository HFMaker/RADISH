CC      = gcc
CFLAGS  = -Wall -Wextra -O2
TARGET  = radish
PREFIX = /usr/local
MAN_DIR = $(PREFIX)/share/man/man1

all: $(TARGET)

$(TARGET): radish.c
	$(CC) $(CFLAGS) -o $(TARGET) radish.c

clean:
	rm -f $(TARGET)

install: all
	install -m 755 $(TARGET)
	gzip -9c radish.1 | install -m 644 /dev/stdin $(MAN_DIR)/$(TARGET).1.gz

uninstall:
	rm -f /usr/bin/$(TARGET)   
