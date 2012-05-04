#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#include <gtk/gtk.h>
#include <gtk-vlc-player.h>

int
main(int argc, char *argv[])
{
	GtkWidget *window, *player;

	/* init threads */
#ifdef HAVE_X11_XLIB_H
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
	gtk_vlc_player_play(GTK_VLC_PLAYER(player));

	gtk_widget_show_all(window);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}