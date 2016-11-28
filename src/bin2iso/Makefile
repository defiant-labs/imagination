BINDIR ?= bin/
SRCDIR ?= ../

bin2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)bin2iso/bin2iso.c -o $(BINDIR)bin2iso

ifndef BINARIES

all: bin2iso

clean:
	rm -f $(BINDIR)bin2iso
	@rmdir $(BINDIR)

.PHONY: all clean

endif
