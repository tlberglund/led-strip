CC=gcc
CFLAGS=-Wall -Wextra

all: spi_test

spi_test: spi_test.c
	$(CC) $(CFLAGS) -o spi_test spi_test.c

clean:
	rm -f spi_test
	