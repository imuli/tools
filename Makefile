targets=$(patsubst %.c, bin/%, $(wildcard *.c))
all: $(targets)

bin/%: %.c | bin
	cc $(CFLAGS) -o $@ $<

bin:
	mkdir bin

clean:
	rm -rf bin
