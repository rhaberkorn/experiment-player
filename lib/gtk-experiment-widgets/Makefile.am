AM_CFLAGS = -Wall

BUILT_SOURCES = cclosure-marshallers.c cclosure-marshallers.h

noinst_LTLIBRARIES = libgtk-experiment-transcript.la
libgtk_experiment_transcript_la_SOURCES = gtk-experiment-transcript.h \
					  gtk-experiment-transcript-private.h \
					  gtk-experiment-transcript.c \
					  gtk-experiment-transcript-formats.c

libgtk_experiment_transcript_la_CFLAGS = $(AM_CFLAGS) \
					 @LIBGTK_CFLAGS@
libgtk_experiment_transcript_la_CPPFLAGS = -I@top_srcdir@/lib/experiment-reader

lib_LTLIBRARIES = libgtk-experiment-widgets.la
libgtk_experiment_widgets_la_SOURCES = gtk-experiment-navigator.h \
				       gtk-experiment-navigator.c
nodist_libgtk_experiment_widgets_la_SOURCES = $(BUILT_SOURCES)

libgtk_experiment_widgets_la_CFLAGS = $(AM_CFLAGS)
libgtk_experiment_widgets_la_CPPFLAGS =
libgtk_experiment_widgets_la_LDFLAGS = -no-undefined -shared -bindir @bindir@ \
				       -avoid-version
libgtk_experiment_widgets_la_LIBADD = libgtk-experiment-transcript.la

libgtk_experiment_widgets_la_CFLAGS += @LIBGTK_CFLAGS@
libgtk_experiment_widgets_la_LIBADD += @LIBGTK_LIBS@

libgtk_experiment_widgets_la_CPPFLAGS += -I@top_srcdir@/lib/experiment-reader
libgtk_experiment_widgets_la_LIBADD += @top_srcdir@/lib/experiment-reader/libexperiment-reader.la

include_HEADERS = gtk-experiment-navigator.h \
		  gtk-experiment-transcript.h

dist_gtk_experiment_widgets_catalogs_DATA = gtk-experiment-widgets-catalog.xml

dist_noinst_DATA = cclosure-marshallers.list
CLEANFILES = $(BUILT_SOURCES)

MARSHAL_PREFIX = gtk_experiment_widgets_marshal

cclosure-marshallers.c : cclosure-marshallers.list
	@GLIB_GENMARSHAL@ --prefix $(MARSHAL_PREFIX) --body $< >$@

cclosure-marshallers.h : cclosure-marshallers.list
	@GLIB_GENMARSHAL@ --prefix $(MARSHAL_PREFIX) --header $< >$@
