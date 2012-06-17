AM_CFLAGS = -Wall

SUBDIRS = . tests

reader_datadir = @datarootdir@/libexperiment-reader

BUILT_SOURCES = cclosure-marshallers.c cclosure-marshallers.h

lib_LTLIBRARIES = libexperiment-reader.la
libexperiment_reader_la_SOURCES = experiment-reader.c experiment-reader.h
nodist_libexperiment_reader_la_SOURCES = $(BUILT_SOURCES)

libexperiment_reader_la_CFLAGS = $(AM_CFLAGS)
libexperiment_reader_la_CPPFLAGS =
libexperiment_reader_la_LDFLAGS = -no-undefined -bindir @bindir@
libexperiment_reader_la_LIBADD =

libexperiment_reader_la_CFLAGS += @LIBGLIB_CFLAGS@
libexperiment_reader_la_LIBADD += @LIBGLIB_LIBS@

libexperiment_reader_la_CFLAGS += @LIBXML2_CFLAGS@
libexperiment_reader_la_LIBADD += @LIBXML2_LIBS@

include_HEADERS = experiment-reader.h

dist_reader_data_DATA = session.dtd

dist_noinst_DATA = cclosure-marshallers.list
CLEANFILES = $(BUILT_SOURCES)

MARSHAL_PREFIX = experiment_reader_marshal

cclosure-marshallers.c : cclosure-marshallers.list
	@GLIB_GENMARSHAL@ --prefix $(MARSHAL_PREFIX) --body $< >$@

cclosure-marshallers.h : cclosure-marshallers.list
	@GLIB_GENMARSHAL@ --prefix $(MARSHAL_PREFIX) --header $< >$@
