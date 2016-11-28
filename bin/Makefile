BINDIR  ?= ./
SRCDIR  ?= ../src/
BINARIES = cdi2iso pdi2iso mdf2iso b5i2iso nrg2iso bin2iso iso2dir

all: $(BINARIES)

cdi2iso:
	gcc $(SRCDIR)cdi2iso/cdi2iso.c -o $(BINDIR)cdi2iso

pdi2iso:
	gcc $(SRCDIR)pdi2iso/pdi2iso.c -o $(BINDIR)pdi2iso

mdf2iso:
	gcc $(SRCDIR)mdf2iso/mdf2iso.c -o $(BINDIR)mdf2iso

b5i2iso:
	gcc $(SRCDIR)b5i2iso/b5i2iso.c -o $(BINDIR)b5i2iso

nrg2iso:
	gcc $(SRCDIR)nrg2iso/nrg2iso.c -o $(BINDIR)nrg2iso

bin2iso:
	gcc $(SRCDIR)bin2iso/bin2iso.c -o $(BINDIR)bin2iso

iso2bin:
	gcc $(SRCDIR)iso2bin/iso2bin.c -o $(BINDIR)iso2bin

iso2dir:
	gcc -lm $(SRCDIR)iso2dir/iso2dir.c -o $(BINDIR)iso2dir

clean:
	rm $(addprefix $(BINDIR), $(BINARIES))

.PHONY: all clean
