AM_CFLAGS = -Wall

bin_PROGRAMS = experiment-player
experiment_player_SOURCES = main.c

experiment_player_CFLAGS =
experiment_player_CPPFLAGS = -DDEFAULT_UI='"@DEFAULT_UI@"'
experiment_player_LDFLAGS =
experiment_player_LDADD =

experiment_player_CPPFLAGS += -I@top_srcdir@/lib/gtk-vlc-player
experiment_player_LDADD += @top_srcdir@/lib/gtk-vlc-player/libgtk-vlc-player.la

experiment_player_CFLAGS += @LIBGTK_CFLAGS@
experiment_player_LDADD += @LIBGTK_LIBS@

experiment_player_CFLAGS += @GTKAPP_CFLAGS@
experiment_player_LDFLAGS += @GTKAPP_LDFLAGS@

dist_player_data_DATA = default.ui
