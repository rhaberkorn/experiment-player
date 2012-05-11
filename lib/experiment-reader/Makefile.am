AM_CFLAGS = -Wall

reader_datadir = @datarootdir@/libexperiment-reader

lib_LTLIBRARIES = libexperiment-reader.la
libexperiment_reader_la_SOURCES = experiment-reader.c experiment-reader.h

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
