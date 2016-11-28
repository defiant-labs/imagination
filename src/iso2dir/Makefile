BINDIR ?= bin/
SRCDIR ?= ../

$(BINDIR)iso2dir:
	@mkdir -p $(BINDIR)
	gcc $(SRCDIR)iso2dir/iso2dir.c -lm -o $(BINDIR)iso2dir

ifndef BINARIES

all: iso2dir

clean:
	rm -f $(BINDIR)iso2dir
	@rmdir $(BINDIR)

.PHONY: clean

endif
