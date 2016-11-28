BINDIR ?= bin/
SRCDIR ?= ../

iso2bin:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)iso2bin/iso2bin.c -o $(BINDIR)iso2bin

ifndef BINARIES

all: iso2bin

clean:
	rm -f $(BINDIR)iso2bin
	@rmdir $(BINDIR)

.PHONY: all clean

endif
