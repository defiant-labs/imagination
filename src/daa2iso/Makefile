BINDIR ?= bin/
SRCDIR ?= ../

daa2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)daa2iso/daa2iso.c -o $(BINDIR)daa2iso

ifndef BINARIES

all: daa2iso

clean:
	rm -f $(BINDIR)daa2iso
	@rmdir $(BINDIR)

.PHONY: all clean

endif
