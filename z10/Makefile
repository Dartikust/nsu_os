CC = cc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2

.PHONY: all run clean

all: z10

z10: z10.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ z10.c $(LDLIBS)

run: z10
	./z10 $(ARGS)

clean:
	rm -f z10 z10.exe *.o
