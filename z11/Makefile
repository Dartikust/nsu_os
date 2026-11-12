CC = cc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2

.PHONY: all run clean

all: z11

z11: z11.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ z11.c $(LDLIBS)

run: z11
	./z11 $(ARGS)

clean:
	rm -f z11 z11.exe *.o
