ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = config.h.in

pkglibdir = $(libdir)/deadbeef

pkglib_LTLIBRARIES = fcdec.la

fcdec_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version -no-undefined
fcdec_la_CFLAGS = $(DECODER_CFLAGS)
fcdec_la_LIBADD = $(DECODER_LIBS)

fcdec_la_SOURCES = fcdec.c

dist-hook:
	@RM@ -f $(distdir)/config.h
