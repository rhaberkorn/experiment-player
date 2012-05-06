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

/* NOTE: for some strange reason the parameters are switched */
void
playpause_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	gtk_button_set_label(GTK_BUTTON(data),
			     gtk_vlc_player_toggle(GTK_VLC_PLAYER(widget))
			     	? "gtk-media-play"
			     	: "gtk-media-pause");
}

/* NOTE: for some strange reason the parameters are switched */
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
	GtkWidget *window, *player, *scale;

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
	scale = GTK_WIDGET(gtk_builder_get_object(builder, "scale_widget"));

	g_object_unref(G_OBJECT(builder));

	/* connect scale with player widget */
	gtk_range_set_adjustment(GTK_RANGE(scale),
				 gtk_vlc_player_get_time_adjustment(GTK_VLC_PLAYER(player)));

	gtk_vlc_player_load(GTK_VLC_PLAYER(player), argv[1]);

	gtk_widget_set_sensitive(GTK_WIDGET(scale), TRUE);

	gtk_widget_show_all(window);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}