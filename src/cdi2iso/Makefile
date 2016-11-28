BINDIR ?= bin/
SRCDIR ?= ../

cdi2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)cdi2iso/cdi2iso.c -o $(BINDIR)cdi2iso

ifndef BINARIES

all: cdi2iso

clean:
	rm -f $(BINDIR)cdi2iso
	@rmdir $(BINDIR)

.PHONY: all clean

endif
