BINDIR ?= bin/
SRCDIR ?= ../

$(BINDIR)nrg2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)nrg2iso/nrg2iso.c -o $(BINDIR)nrg2iso

ifndef BINARIES

all: nrg2iso

clean:
	rm -f $(BINDIR)nrg2iso
	@rmdir $(BINDIR)

.PHONY: clean

endif
