#   Copyright (C) 2007,2008,2009 Luigi Auriemma
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   http://www.gnu.org/licenses/gpl.txt

BINDIR ?= bin/
SRCDIR ?= ../

CFLAGS     += -O2 -s

SRC         = $(SRCDIR)uif2iso/uif2iso.c
LIBS        = $(addprefix $(SRCDIR)uif2iso/, des.c LzmaDec.c Bra86.c)
LIBSNEW     = $(addprefix $(SRCDIR)uif2iso/, 3way.c blowfish.c dunno.c gost.c idea.c loki91.c rc5.c seal.c)

$(BINDIR)uif2iso:
	@mkdir -p $(BINDIR)
	$(CC) $(SRC) $(CFLAGS) -o $(BINDIR)uif2iso -lz $(LIBS) $(LIBSNEW) -DMAGICISO_IS_SHIT

ifndef BINARIES

all: uif2iso

clean:
	rm -f $(BINDIR)uif2iso
	@rmdir $(BINDIR)

.PHONY: clean install

endif

# End of file
