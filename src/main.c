#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#include <gtk/gtk.h>
#include <gtk-vlc-player.h>

static GtkWidget *window,
		 *player,
		 *scale;

/*
 * GtkBuilder signal callbacks
 * NOTE: for some strange reason the parameters are switched
 */

void
playpause_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *image = gtk_bin_get_child(GTK_BIN(data));
	gboolean is_playing;

	is_playing = gtk_vlc_player_toggle(GTK_VLC_PLAYER(widget));
	gtk_image_set_from_stock(GTK_IMAGE(image),
				 is_playing ? "gtk-media-play"
					    : "gtk-media-pause",
				 GTK_ICON_SIZE_SMALL_TOOLBAR);
}

void
stop_button_clicked_cb(GtkWidget *widget,
		       gpointer data __attribute__((unused)))
{
	gtk_vlc_player_stop(GTK_VLC_PLAYER(widget));
}

void
file_menu_openmovie_item_activate_cb(GtkWidget *widget,
				     gpointer data __attribute__((unused)))
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Open Movie...", GTK_WINDOW(widget),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					     NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));

		if (gtk_vlc_player_load(GTK_VLC_PLAYER(player), uri)) {
			/* TODO */
		} else {
			gtk_widget_set_sensitive(scale, TRUE);
		}

		g_free(uri);
	}

	gtk_widget_destroy(dialog);
}

void
file_menu_opentranscript_item_activate_cb(GtkWidget *widget, gpointer data)
{
	/* TODO */
}

void
quickopen_menu_choosedir_item_activate_cb(GtkWidget *widget, gpointer data)
{
	/* TODO */
}

int
main(int argc, char *argv[])
{
	GtkBuilder *builder;

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

	gtk_widget_show_all(window);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}