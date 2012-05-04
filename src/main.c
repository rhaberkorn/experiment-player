#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#include <gtk/gtk.h>
#include <gtk-vlc-player.h>

/*
 * GtkBuilder signal callbacks
 */

void
playpause_button_clicked_cb(GtkWidget *widget,
			    gpointer data __attribute__((unused)))
{
	gtk_vlc_player_toggle(GTK_VLC_PLAYER(widget));
}

void
stop_button_clicked_cb(GtkWidget *widget,
		       gpointer data __attribute__((unused)))
{
	gtk_vlc_player_stop(GTK_VLC_PLAYER(widget));
}


int
main(int argc, char *argv[])
{
	GtkBuilder *builder;
	GtkWidget *window, *player;

	/* init threads */
#ifdef HAVE_X11_XLIB_H
	XInitThreads(); /* FIXME: really required??? */
#endif
	g_thread_init(NULL);
	gdk_threads_init();

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();

	gtk_builder_add_from_file(builder, DEFAULT_UI, NULL);
	gtk_builder_connect_signals(builder, NULL);

	window = GTK_WIDGET(gtk_builder_get_object(builder, "player_window"));
	player = GTK_WIDGET(gtk_builder_get_object(builder, "player_widget"));

	g_object_unref(builder);

	gtk_vlc_player_load(GTK_VLC_PLAYER(player), argv[1]);

	gtk_widget_show_all(window);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}