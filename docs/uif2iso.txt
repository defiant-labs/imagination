######################################################################

Title:  UIF2ISO
Author: Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

UIF2ISO homepage:
        http://aluigi.org/mytoolz.htm#uif2iso

######################################################################

1) Introduction
2) Shame on MagicISO and GPL violation
3) Usage on Windows
4) Usage on *nix/MacOSX
5) Features and known bugs
6) Technical info about the format
7) Comments about the UIF format

######################################################################

===============
1) Introduction
===============

UIF2ISO is an open source command-line/GUI tool for converting single
and multipart UIF images to the original ISO format.
It's released under the GNU GPL license and its source code is
available in the src folder.

The UIF image (Universal Image Format, although there is nothing of
"universal" in it) in fact is just a compressed CD/DVD image created
through a commercial program called MagicISO.


######################################################################

======================================
2) Shame on MagicISO and GPL violation
======================================

The chinese program called MagicISO has created the UIF format in the
2006 and, exactly like PowerISO already did at the beginning of that
year, this format has been slowly started to be used to re-diffuse
copyrighted material through BitTorrent... with the right effect that
anyone hates this format (which is totally useless and, being closed,
it should be NOT used on file sharing networks).

This is a shameful marketing strategy made by the same author of
MagicISO (a certain Gang JiaPing) for forcing the users who download
these torrents to buy and use his program.

These facts have been confirmed just recently due to the introduction
of various senseless encryption mechanisms in the UIF files released
on BitTorrent, all encryptions which have nothing to do with the UIF
format but have been inserted only for trying to avoid the usage of my
uif2iso alternative... without success 8-)

For example the recent 5.5.272 version of MagicISO has introduced well
"8" customized encryption algorithms plus other modifications JUST
with the target of making the UIF format unreadable (again without
success, look at uif2iso 0.1.6) and has also introduced support for
the DAA image format.
Unfortunately most of the code used by MagicISO for handling the DAA
format has been "taken" from my other open source daa2iso tool
violating my GPL license and showing how much ehmmmm "unprofessional"
is the author of that program:

  http://aluigi.org/misc/magiciso_gpl_violation.txt

Luckily that code which was copied was old and not compatible with the
newer versions of the DAA format so it fails to handle these files
correctly.

My best suggestion is to spread the voice about the shameful actions
of MagicISO and its author and avoiding to download or keep the
seeding of torrents of UIF or DAA images.

And remember that uif2iso has been created only for supporting the UIF
format and *NOT* for playing "cat and mouse" with an idiot who
modifies this stupid zipped format each time for becoming the only
program able to read it...


######################################################################

===================
3) Usage on Windows
===================

Using UIF2ISO on Windows is really a joke, just double click on
UIF2ISO.exe and the tool will open a DOS-like window which contains
all the needed informations about the status of conversion, then you
will need to choose the input UIF file you want to convert and
subsequently the name of the output file you want to create.

Note that the correct extension (ISO, NRG or BIN/CUE and so on) is
automatically selected by the tool depending by the input UIF file.

If you want to use the tool from the command-line, so specifying the
input and output files manually as in the older versions of the tool,
you can do it too since UIF2ISO automatically recognizes if it has
been launched from the console (cmd.exe) or through double-click.
Just specify the input UIF file and the output ISO file you want to
create like in the examples of the subsequent section.

Remember that you can also associate the UIF extension to UIF2ISO, so
when you will double-click on these files UIF2ISO will popup and will
allow you to choose the output file immediately or you can also
drag'n'drop the UIF file directly on UIF2ISO.EXE.

Note that UIF2ISO is a stand-alone program, so all you need to have is
only UIF2ISO.EXE and you can place it everywhere you want.


######################################################################

=======================
4) Usage on *nix/MacOSX
=======================

Compile the source code using 'make', this will generate the UIF2ISO
executable.
If you want to install it type 'make install' or just copy the
executable where you want since it's the only file you need.

The only requirement for compiling and using UIF2ISO is zlib
(apt-get install zlib1g zlib1g-dev).

Using it then it's simple, just specify the input file and the ISO
file you want to create like the following example:

  uif2iso "my file.uif" output.iso


######################################################################

==========================
5) Features and known bugs
==========================

The tool supports password/encryption, little/big endian architectures
and should work on many platforms (Windows, Linux, MacOS, *BSD, Amiga
and others).

The NRG file (compressed UIF) created by MagicISO is invalid so,
although my tool tries to fix some fields and generates an additional
CUE file, is possible that the resulted image isn't fully recognized
by the burning/mounting program you want to use.
During my tests I have found (only one time) also an invalid output
ISO file (the uif was decompressed with both uif2iso and MagicISO)
which was possible to read correctly only through MagicISO... it's
useless to say that is a shame since creating non-standard images is
an INTENTIONAL action made by the MagicISO's programmer!!!

A known micro-bug is that on Windows 95/98/ME works only the so
called GUI version because the trick I use to know if the program has
been launched from the console or through double-click is not
compatible with this OS, anyway this is not a problem because the 99%
of the Windows users don't like the command-line 8-)

I'm available for any comment or feedback, so if you find a
compatibility problem with a specific UIF image (and you are sure that
the image is perfect) send me a mail!!!


######################################################################

==================================
6) Technical info about the format
==================================

UIF2ISO is open source so there is nothing better than its source code
for explaining in detail this file format and for updates.

In short UIF is a compressed disk image which can be derived by 5 or
more types of input images:
- ISO: only the classical data
- BIN/CUE: data, audio or mixed content
  the UIF file contains a BLMS section with raw CUE informations
  (UIF2ISO uses them to generate the relative CUE file) and a BLSS
  field containing the original CUE file used by who created the UIF
- MDS/MDF: Alcohol 120%'s proprietary disk image format
- CCD: CloneCD image
- NRG: disk image in Nero v2 format
  unfortunately for some reasons which I don't know the "UIFed" NRG
  image generated by MagicISO is invalid (these images don't work with
  other programs included Nero which is the creator of the NRG
  format!) and although my UIF2ISO fixes some of these fields is still
  possible that with some type of images the fixed NRG could not work
  since not 100% compatible.
  Another work-around I have adopted is the generation of an
  additional CUE file derived from the NRG one which in my test worked
  perfectly with data, audio and mixed CDs and partially with CD
  extra.


######################################################################

================================
7) Comments about the UIF format
================================

I don't like and don't approve the UIF format because it's proprietary
and doesn't give benefits.
What you can do with UIF can be done better with ZIP or 7zip without
the need to be forced to buy a software like MagicISO only for burning
or converting a CD/DVD image.

Ok exists my tool which can do the job but this is not a valid reason
to continue to use this useless format.

So if you want to create a CD/DVD image, DO NOT USE UIF!


######################################################################
