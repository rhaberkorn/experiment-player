#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#include <shellapi.h>
#endif

#include <glib.h>

#include <gtk/gtk.h>

#include <gtk-vlc-player.h>
#include <gtk-experiment-navigator.h>
#include <experiment-reader.h>

#include "experiment-player.h"

static inline void button_image_set_from_stock(GtkButton *widget, const gchar *name);

GtkWidget *player_window;

GtkWidget *player_widget,
	  *controls_hbox,
	  *scale_widget,
	  *playpause_button,
	  *volume_button;

gchar *current_filename = NULL;

/*
 * GtkBuilder signal callbacks
 * NOTE: for some strange reason the parameters are switched
 */

void
playpause_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	gboolean is_playing = gtk_vlc_player_toggle(GTK_VLC_PLAYER(widget));

	button_image_set_from_stock(GTK_BUTTON(data),
				    is_playing ? "gtk-media-play"
					       : "gtk-media-pause");
}

void
stop_button_clicked_cb(GtkWidget *widget,
		       gpointer data __attribute__((unused)))
{
	gtk_vlc_player_stop(GTK_VLC_PLAYER(widget));
	button_image_set_from_stock(GTK_BUTTON(playpause_button),
				    "gtk-media-play");
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
		gchar *file;

		file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		if (load_media_file(file)) {
			/* TODO */
		}
		refresh_quickopen_menu(GTK_MENU(quickopen_menu));

		g_free(file);
	}

	gtk_widget_destroy(dialog);
}

void
file_menu_opentranscript_item_activate_cb(GtkWidget *widget, gpointer data)
{
	/* TODO */
}

void
help_menu_manual_item_activate_cb(GtkWidget *widget __attribute__((unused)),
				  gpointer data __attribute__((unused)))
{
	GError *err = NULL;
	gboolean res;

#ifdef HAVE_WINDOWS_H
	res = (int)ShellExecute(NULL, "open", HELP_URI,
				NULL, NULL, SW_SHOWNORMAL) <= 32;
	if (res)
		err = g_error_new(g_quark_from_static_string("Cannot open!"), 0,
				  "Cannot open '%s'!", HELP_URI);
#else
	res = !gtk_show_uri(NULL, HELP_URI, GDK_CURRENT_TIME, &err);
#endif

	if (res) {
		show_message_dialog_gerror(err);
		g_error_free(err);
	}
}

void
generic_quit_cb(GtkWidget *widget __attribute__((unused)),
		gpointer data __attribute__((unused)))
{
	gtk_main_quit();
}

static inline void
button_image_set_from_stock(GtkButton *widget, const gchar *name)
{
	GtkWidget *image = gtk_bin_get_child(GTK_BIN(widget));

	gtk_image_set_from_stock(GTK_IMAGE(image), name,
				 GTK_ICON_SIZE_SMALL_TOOLBAR);
}

gboolean
load_media_file(const gchar *file)
{
	if (gtk_vlc_player_load_filename(GTK_VLC_PLAYER(player_widget), file))
		return TRUE;

	g_free(current_filename);
	current_filename = g_strdup(file);

	gtk_widget_set_sensitive(controls_hbox, TRUE);

	button_image_set_from_stock(GTK_BUTTON(playpause_button),
				    "gtk-media-play");

	return FALSE;
}

void
show_message_dialog_gerror(GError *err)
{
	GtkWidget *dialog;

	if (err == NULL)
		return;

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"%s", err->message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

int
main(int argc, char *argv[])
{
	GtkBuilder *builder;
	GtkAdjustment *adj;

	/* FIXME: support internationalization instead of enforcing English */
#ifdef __WIN32__
	g_setenv("LC_ALL", "English", TRUE);
#else
	g_setenv("LC_ALL", "en", TRUE);
#endif
	setlocale(LC_ALL, "");

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

	BUILDER_INIT(builder, player_window);

	BUILDER_INIT(builder, player_widget);
	BUILDER_INIT(builder, controls_hbox);
	BUILDER_INIT(builder, scale_widget);
	BUILDER_INIT(builder, playpause_button);
	BUILDER_INIT(builder, volume_button);

	BUILDER_INIT(builder, quickopen_menu);
	BUILDER_INIT(builder, quickopen_menu_empty_item);

	g_object_unref(G_OBJECT(builder));

	/* connect timeline and volume button with player widget */
	adj = gtk_vlc_player_get_time_adjustment(GTK_VLC_PLAYER(player_widget));
	gtk_range_set_adjustment(GTK_RANGE(scale_widget), adj);
	adj = gtk_vlc_player_get_volume_adjustment(GTK_VLC_PLAYER(player_widget));
	gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON(volume_button), adj);

	quickopen_directory = g_strdup(DEFAULT_QUICKOPEN_DIR);
	refresh_quickopen_menu(GTK_MENU(quickopen_menu));

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}