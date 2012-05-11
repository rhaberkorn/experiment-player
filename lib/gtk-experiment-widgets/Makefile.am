AM_CFLAGS = -Wall

lib_LTLIBRARIES = libgtk-experiment-widgets.la
libgtk_experiment_widgets_la_SOURCES = gtk-experiment-navigator.c \
				       gtk-experiment-navigator.h

libgtk_experiment_widgets_la_CFLAGS = $(AM_CFLAGS)
libgtk_experiment_widgets_la_CPPFLAGS =
libgtk_experiment_widgets_la_LDFLAGS = -no-undefined -shared -bindir @bindir@ \
				       -avoid-version
libgtk_experiment_widgets_la_LIBADD =

libgtk_experiment_widgets_la_CFLAGS += @LIBGTK_CFLAGS@
libgtk_experiment_widgets_la_LIBADD += @LIBGTK_LIBS@

libgtk_experiment_widgets_la_CPPFLAGS += -I@top_srcdir@/lib/experiment-reader
#libgtk_experiment_widgets_la_LIBADD += @top_srcdir@/lib/experiment-reader/libexperiment-reader.la

include_HEADERS = gtk-experiment-navigator.h

dist_player_data_DATA = gtk-experiment-widgets-catalog.xml
