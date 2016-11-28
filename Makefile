BINDIR   = bin/
SRCDIR   = src/
BINARIES = cdi2iso b5i2iso pdi2iso mdf2iso nrg2iso bin2iso iso2dir

all: $(BINARIES)

clean:
	rm -f $(addprefix $(BINDIR), $(BINARIES))
	@rmdir $(BINDIR)

include $(addprefix $(SRCDIR), $(addsuffix /Makefile, $(BINARIES)))

.PHONY: all clean
