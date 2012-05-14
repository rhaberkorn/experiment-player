AM_CFLAGS = -Wall
AM_CPPFLAGS = -I..
LDADD = ../libexperiment-reader.la

AM_CFLAGS += @LIBGLIB_CFLAGS@
LDADD += @LIBGLIB_LIBS@

check_PROGRAMS = unit-tests
dist_noinst_DATA = test-experiment-valid.xml

if USE_GTESTER
check-local : gtester-log.html
endif

gtester-log.html : gtester-log.xml
	@GTESTER_REPORT@ $< >$@

gtester-log.xml : $(check_PROGRAMS)
	@GTESTER@ -m=quick -o=$@ $^

CLEANFILES = gtester-log.xml gtester-log.html
