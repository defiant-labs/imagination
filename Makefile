ifdef SystemRoot #Windows
.PHONY: all all-before all-after clean clean-custom

all: all-before cdi2iso all-after
  
cdi2iso: src/cdi2iso.o
        gcc.exe src/cdi2iso.o -o "bin/cdi2iso.exe"
src/cdi2iso.o: src/cdi2iso.c
        gcc.exe -c src/cdi2iso.c -o src/cdi2iso.o

clean: clean-custom
        rm -f src/cdi2iso.o bin/cdi2iso.exe
else #Unix
all: cdi2iso pdi2iso mdf2iso b5i2iso nrg2iso bin2iso iso2dir

cdi2iso: ./src/cdi2iso.c
	mkdir -p bin; gcc ./src/cdi2iso.c -o bin/cdi2iso
pdi2iso: ./src/pdi2iso.c
	mkdir -p bin; gcc ./src/pdi2iso.c -o bin/pdi2iso
mdf2iso: ./src/mdf2iso.c
	mkdir -p bin; gcc ./src/mdf2iso.c -o bin/mdf2iso
b5i2iso: ./src/b5i2iso.c
	mkdir -p bin; gcc ./src/b5i2iso.c -o bin/b5i2iso
nrg2iso: ./src/nrg2iso.c
	mkdir -p bin; gcc ./src/nrg2iso.c -o bin/nrg2iso
bin2iso: ./src/bin2iso.c
	mkdir -p bin; gcc ./src/bin2iso.c -o bin/bin2iso
#img2iso, daa2iso, uif2iso, bwi cif? FCD GCD PO1, C2D, ccd2iso

#iso2bin: ./src/iso2bin.c ./src/EdcEcc.c
#	mkdir -p bin; gcc ./src/iso2bin.c -o bin/iso2bin

iso2dir: ./src/iso2dir.c
	mkdir -p bin; gcc -lm ./src/iso2dir.c -o bin/iso2dir

install:
	cp -v bin/* /usr/bin
	ldconfig
#uninstall:

clean: 
	rm -rf bin
endif
