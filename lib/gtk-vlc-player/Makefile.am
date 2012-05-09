AM_CFLAGS = -Wall

vlcplayer_datadir = @datarootdir@/gtk-vlc-player

lib_LTLIBRARIES = libgtk-vlc-player.la
libgtk_vlc_player_la_SOURCES = gtk-vlc-player.c gtk-vlc-player.h

libgtk_vlc_player_la_CFLAGS = $(AM_CFLAGS) \
			      @LIBGTK_CFLAGS@ @LIBVLC_CFLAGS@
libgtk_vlc_player_la_LIBADD = @LIBGTK_LIBS@ @LIBVLC_LIBS@
libgtk_vlc_player_la_LDFLAGS = -no-undefined -shared -module

include_HEADERS = gtk-vlc-player.h

dist_vlcplayer_data_DATA = gtk-vlc-player-catalog.xml
