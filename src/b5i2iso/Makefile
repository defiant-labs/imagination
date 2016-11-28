BINDIR ?= bin/
SRCDIR ?= ../

b5i2iso:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)b5i2iso/b5i2iso.c -o $(BINDIR)b5i2iso

ifndef BINARIES

all: b5i2iso

clean:
	rm -f $(BINDIR)b5i2iso
	@rmdir $(BINDIR)

.PHONY: all clean

endif
