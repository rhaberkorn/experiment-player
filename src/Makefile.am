AM_CFLAGS = -Wall

bin_PROGRAMS = experiment-player
experiment_player_SOURCES = main.c quickopen.c \
			    experiment-player.h

experiment_player_CFLAGS = $(AM_CFLAGS)
experiment_player_CPPFLAGS = -DDEFAULT_UI='"@DEFAULT_UI@"' \
			     -DHELP_URI='"@HELP_URI@"'
experiment_player_LDFLAGS = -export-dynamic
experiment_player_LDADD =

if USE_W32_RES
experiment_player_LDADD += icon.$(OBJEXT)
endif

experiment_player_CFLAGS += @LIBGTK_CFLAGS@
experiment_player_LDADD += @LIBGTK_LIBS@

experiment_player_CPPFLAGS += -I@top_srcdir@/lib/gtk-vlc-player
experiment_player_LDADD += @top_srcdir@/lib/gtk-vlc-player/libgtk-vlc-player.la

experiment_player_CPPFLAGS += -I@top_srcdir@/lib/gtk-experiment-widgets
experiment_player_LDADD += @top_srcdir@/lib/gtk-experiment-widgets/libgtk-experiment-widgets.la

experiment_player_CPPFLAGS += -I@top_srcdir@/lib/experiment-reader
#experiment_player_LDADD += @top_srcdir@/lib/experiment-reader/libexperiment-reader.la

experiment_player_CFLAGS += @GTKAPP_CFLAGS@
experiment_player_LDFLAGS += @GTKAPP_LDFLAGS@

dist_player_data_DATA = default.ui \
			experiment-player.ico

EXTRA_DIST = icon.rc

icon.$(OBJEXT) : icon.rc experiment-player.ico
	@RC@ $< $@
