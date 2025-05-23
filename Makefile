# Makefile to build and install unzip and related tools.
#
# This was cribbed from unix/Makefile, but with a lot of complexity removed
# to make it easier to override compiler and linker flags.
#
#
# BUILDING
# --------
#
# % make
#
# (There is a large number of precompiler definitions that can be used to
# customize the code that gets compiled. See old-docs/INSTALL for a
# nonexhaustive list.)
#
# To build with bzip2 support:
#
# % make CFLAGS_EXTRA=-DUSE_BZIP2 LDLIBS_EXTRA=-lbz2
#
#
# TESTING
# -------
#
# % make check
#
#
# INSTALLING
# ----------
#
# % make install
#
# will install under the default prefix (/usr/local/). To build packages,
# it's useful to specify a custom prefix and use the DESTDIR variable to
# install in a subdirectory which can then be packaged up:
#
# % make prefix=/usr
# % make prefix=/usr DESTDIR=pkg install
#
# will install into the pkg/ subdirectory, which can then be packaged.
#
# Note that the install target currently does not strip installed binaries!
# This is useful for packaging tools that want to preserve debug symbols.
# To strip symbols manually, use something like:
#
# % strip pkg/usr/local/bin/*
#

prefix = /usr/local
manext = 1

CC = cc
LD = ld
INSTALL = install
RM = rm
LN = ln

# Optimization flags.
# -O2 is a good tradeoff between compilation speed and performance.
CFLAGS_OPTIMIZATIONS = -O2

# Compiler diagnostics to enable.
CFLAGS_DIAGNOSTICS = -Wall -Wno-old-style-definition -Werror=implicit-function-declaration

# These correspond roughly to GCC's -fhardened, but they're spelled out here
# for compatibility with clang which currently doesn't support that option.
CFLAGS_SECURITY = \
	-D_FORTIFY_SOURCE=3 \
	-Wl,-z,relro,-z,now,-z,nodlopen,-z,noexecstack \
	-fPIE -pie \
	-fstack-protector-strong \
	-fstack-clash-protection

# Commonly enabled features:
#
#  -DNO_LCHMOD: POSIX/Linux does not have lchmod(), though BSD/OS X does
#  -DDATE_FORMAT=DF_YMD: switch dates from American d/m/y to ISO y/m/d
#
CFLAGS_FEATURES = \
    -DNO_LCHMOD \
    -DDATE_FORMAT=DF_YMD

# Convenience variable to append additional CFLAGS on the command line.
CFLAGS_EXTRA =

CFLAGS = $(CFLAGS_OPTIMIZATIONS) $(CFLAGS_DIAGNOSTICS) $(CFLAGS_SECURITY) $(CFLAGS_FEATURES) $(CFLAGS_EXTRA)

# Convenience variable to append additional LDFLAGS on the command line.
LDFLAGS_EXTRA =

LDFLAGS = $(LDFLAGS_EXTRA)

# Convenience variable to append additional LDFLAGS on the command line.
LDLIBS_EXTRA =

LDLIBS = $(LDLIBS_EXTRA)

BINDIR = $(prefix)/bin
MANDIR = $(prefix)/man/man$(manext)

E =
O = .o
M = unix

UNZIPS = unzip$E funzip$E unzipsfx$E

BINS = $(UNZIPS) zipinfo

MANS = man/funzip.1 man/unzip.1 man/unzipsfx.1 man/zipgrep.1 man/zipinfo.1

DOCS = funzip.txt unzip.txt unzipsfx.txt zipgrep.txt zipinfo.txt

OBJS1 = unzip$O crc32$O crypt$O envargs$O explode$O
OBJS2 = extract$O fileio$O globals$O inflate$O list$O match$O
OBJS3 = process$O ttyio$O ubz2err$O unshrink$O zipinfo$O
OBJS = $(OBJS1) $(OBJS2) $(OBJS3) $M$O

OBJX = unzipsfx$O crc32_$O crypt_$O extract_$O fileio_$O \
	globals_$O inflate_$O match_$O process_$O ttyio_$O ubz2err_$O $M_$O

OBJF = funzip$O crc32$O cryptf$O globalsf$O inflatef$O ttyiof$O

UNZIP_H = unzip.h unzpriv.h globals.h unix/unxcfg.h


all: $(BINS)

check: all
	tests/run-tests.sh

install: $(UNZIPS) $(MANS)
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) $(UNZIPS) "$(DESTDIR)$(BINDIR)"
	$(INSTALL) unix/zipgrep "$(DESTDIR)$(BINDIR)"
	$(RM) -f "$(DESTDIR)$(BINDIR)"/zipinfo
	$(LN) "$(DESTDIR)$(BINDIR)/unzip" "$(DESTDIR)$(BINDIR)/zipinfo"

	$(INSTALL) -d "$(DESTDIR)$(MANDIR)"
	$(INSTALL) -m0644 man/funzip.1 "$(DESTDIR)$(MANDIR)/funzip.$(manext)"
	$(INSTALL) -m0644 man/unzip.1 "$(DESTDIR)$(MANDIR)/unzip.$(manext)"
	$(INSTALL) -m0644 man/unzipsfx.1 "$(DESTDIR)$(MANDIR)/unzipsfx.$(manext)"
	$(INSTALL) -m0644 man/zipgrep.1 "$(DESTDIR)$(MANDIR)/zipgrep.$(manext)"
	$(INSTALL) -m0644 man/zipinfo.1 "$(DESTDIR)$(MANDIR)/zipinfo.$(manext)"

clean:
	rm -f $(OBJS) $(OBJX) $(OBJF) $(BINS)

docs: $(DOCS)

# Verifies that installed files match expectations. This is mainly intended to be used
# by the test automation, but could be used by packagers too.
check-install: tests/testdata/installed-files.txt
	test -n "$(DESTDIR)"  # check-install only makes sense if DESTDIR is set
	(cd "$(DESTDIR)/" && find . -printf '%M %n %p\n' | sort -k3) \
		| diff - tests/testdata/installed-files.txt

%.txt: man/%.1
	groff -C -Tascii -P -cbou -man $< > $@

unzip$E:	$(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

zipinfo$E:	unzip$E
	$(RM) -f $@
	$(LN) $< $@

unzipsfx$E:	$(OBJX)
	$(CC) $(LDFLAGS) $(OBJX) $(LDLIBS) -o $@

funzip$E:	$(OBJF)
	$(CC) $(LDFLAGS) $(OBJF) $(LDLIBS) -o $@


crc32$O:	crc32.c $(UNZIP_H) zip.h crc32.h
crypt$O:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
envargs$O:	envargs.c $(UNZIP_H)
explode$O:	explode.c $(UNZIP_H)
extract$O:	extract.c $(UNZIP_H) crc32.h crypt.h
fileio$O:	fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
funzip$O:	funzip.c $(UNZIP_H) crc32.h crypt.h ttyio.h
globals$O:	globals.c $(UNZIP_H)
inflate$O:	inflate.c inflate.h $(UNZIP_H)
list$O:		list.c $(UNZIP_H)
match$O:	match.c $(UNZIP_H)
process$O:	process.c $(UNZIP_H) crc32.h
ttyio$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
ubz2err$O:	ubz2err.c $(UNZIP_H)
unshrink$O:	unshrink.c $(UNZIP_H)
unzip$O:	unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
zipinfo$O:	zipinfo.c $(UNZIP_H)

unix$O:		unix/unix.c $(UNZIP_H) unzvers.h
	$(CC) -c -I. $(CFLAGS) -o $@ unix/unix.c

# unzipsfx compilation section
unzipsfx$O:	unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ unzip.c

crc32_$O:	crc32.c $(UNZIP_H) zip.h crc32.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ crc32.c

crypt_$O:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ crypt.c

extract_$O:	extract.c $(UNZIP_H) crc32.h crypt.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ extract.c

fileio_$O:	fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ fileio.c

globals_$O:	globals.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DSFX -o $@ globals.c

inflate_$O:	inflate.c inflate.h $(UNZIP_H) crypt.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ inflate.c

match_$O:	match.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DSFX -o $@ match.c

process_$O:	process.c $(UNZIP_H) crc32.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ process.c

ttyio_$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ ttyio.c

ubz2err_$O:	ubz2err.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DSFX -o $@ ubz2err.c

unix_$O:	unix/unix.c $(UNZIP_H)				# Unix unzipsfx
	$(CC) -c -I. $(CFLAGS) -DSFX -o $@ unix/unix.c

# funzip compilation section
cryptf$O:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ crypt.c

globalsf$O:	globals.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ globals.c

inflatef$O:	inflate.c inflate.h $(UNZIP_H) crypt.h
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ inflate.c

ttyiof$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ ttyio.c

.PHONY: all check clean docs
