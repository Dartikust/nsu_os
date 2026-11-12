CC = cc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2

.PHONY: all run clean

all: z9 z9_nowait

z9: z9.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ z9.c $(LDLIBS)

z9_nowait: z9_nowait.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ z9_nowait.c $(LDLIBS)

run: z9
	./z9 $(ARGS)

clean:
	rm -f z9 z9_nowait z9.exe z9_nowait.exe *.o
