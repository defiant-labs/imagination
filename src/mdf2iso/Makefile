BINDIR ?= bin/
SRCDIR ?= ../

mdf2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)mdf2iso/mdf2iso.c -o $(BINDIR)mdf2iso

ifndef BINARIES

all: mdf2iso

clean:
	rm -f $(BINDIR)mdf2iso
	@rmdir $(BINDIR)

.PHONY: all clean

endif
