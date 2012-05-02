#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <gtk/gtk.h>
#ifdef __WIN32__
#include <gdk/gdkwin32.h>
#else
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif

#include <gtk-vlc-player.h>

int
main(int argc, char *argv[])
{
	GtkWidget *window, *player;

	/* init threads */
#ifndef __WIN32__
	XInitThreads(); /* FIXME: really required??? */
#endif
	g_thread_init(NULL);
	gdk_threads_init();

	gtk_init(&argc, &argv);

#if 0
	GtkBuilder *builder = gtk_builder_new();

	gtk_builder_add_from_file(builder, file, NULL);
	gtk_builder_connect_signals(builder, NULL);
#endif

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	player = gtk_vlc_player_new();
	gtk_container_add(GTK_CONTAINER(window), player);

	gtk_vlc_player_load(GTK_VLC_PLAYER(player), argv[1]);

	gtk_widget_show_all(window);

	gtk_vlc_player_play(GTK_VLC_PLAYER(player));
	
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}