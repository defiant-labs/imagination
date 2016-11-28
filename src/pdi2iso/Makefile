BINDIR ?= bin/
SRCDIR ?= ../

pdi2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)pdi2iso/pdi2iso.c -o $(BINDIR)pdi2iso

ifndef BINARIES

all: pdi2iso

clean:
	rm -f $(BINDIR)pdi2iso
	@rmdir $(BINDIR)

.PHONY: all clean

endif
