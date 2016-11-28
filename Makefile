BINDIR   = bin/
SRCDIR   = src/
BINARIES = bin2iso uif2iso
#cdi2iso b5i2iso pdi2iso mdf2iso nrg2iso iso2dir

all: $(addprefix $(BINDIR), $(BINARIES))

clean:
	rm -f $(addprefix $(BINDIR), $(BINARIES))
	@rmdir $(BINDIR)

include $(addprefix $(SRCDIR), $(addsuffix /Makefile, $(BINARIES)))

.PHONY: clean
