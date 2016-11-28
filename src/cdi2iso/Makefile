BINDIR ?= bin/
SRCDIR ?= ../

$(BINDIR)cdi2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)cdi2iso/cdi2iso.c -o $(BINDIR)cdi2iso

ifndef BINARIES

all: cdi2iso

clean:
	rm -f $(BINDIR)cdi2iso
	@rmdir $(BINDIR)

.PHONY: clean

endif
