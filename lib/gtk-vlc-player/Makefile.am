AM_CFLAGS = -Wall

BUILT_SOURCES = cclosure-marshallers.c cclosure-marshallers.h

lib_LTLIBRARIES = libgtk-vlc-player.la
libgtk_vlc_player_la_SOURCES = gtk-vlc-player.c gtk-vlc-player.h
nodist_libgtk_vlc_player_la_SOURCES = $(BUILT_SOURCES)

libgtk_vlc_player_la_CFLAGS = $(AM_CFLAGS) \
			      @LIBGTK_CFLAGS@ @LIBVLC_CFLAGS@
libgtk_vlc_player_la_LIBADD = @LIBGTK_LIBS@ @LIBVLC_LIBS@
libgtk_vlc_player_la_LDFLAGS = -no-undefined -shared -bindir @bindir@ \
			       -avoid-version

include_HEADERS = gtk-vlc-player.h

dist_gtk_vlc_player_catalogs_DATA = gtk-vlc-player-catalog.xml

dist_noinst_DATA = cclosure-marshallers.list
CLEANFILES = $(BUILT_SOURCES)

MARSHAL_PREFIX = gtk_vlc_player_marshal

cclosure-marshallers.c : cclosure-marshallers.list
	@GLIB_GENMARSHAL@ --prefix $(MARSHAL_PREFIX) --body $< >$@

cclosure-marshallers.h : cclosure-marshallers.list
	@GLIB_GENMARSHAL@ --prefix $(MARSHAL_PREFIX) --header $< >$@
