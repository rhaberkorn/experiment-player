#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>
#include <gtk-vlc-player.h>

static inline gboolean quickopen_filter(const gchar *name);
static gint quickopen_item_cmp(gconstpointer a, gconstpointer b);
static void refresh_quickopen_menu(GtkMenu *menu);
static void reconfigure_all_check_menu_items_cb(GtkWidget *widget, gpointer user_data);
static void quickopen_item_on_activate(GtkWidget *widget, gpointer user_data);

static inline void button_image_set_from_stock(GtkButton *widget, const gchar *name);
static gboolean load_media_file(const gchar *uri);

#define BUILDER_INIT(BUILDER, VAR) do {					\
	VAR = GTK_WIDGET(gtk_builder_get_object(BUILDER, #VAR));	\
} while (0)

static GtkWidget *player_window;

static GtkWidget *player_widget,
		 *controls_hbox,
		 *scale_widget,
		 *playpause_button,
		 *volume_button;

static GtkWidget *quickopen_menu,
		 *quickopen_menu_empty_item;

static gchar *current_filename = NULL;
static gchar *quickopen_directory;

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
		gchar *uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));

		if (load_media_file(uri)) {
			/* TODO */
		}
		refresh_quickopen_menu(GTK_MENU(quickopen_menu));

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
quickopen_menu_choosedir_item_activate_cb(GtkWidget *widget,
					  gpointer data __attribute__((unused)))
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Choose Directory...", GTK_WINDOW(widget),
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					     NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
					    quickopen_directory);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		g_free(quickopen_directory);
		quickopen_directory =
			gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		refresh_quickopen_menu(GTK_MENU(quickopen_menu));
	}

	gtk_widget_destroy(dialog);
}

void
quickopen_menu_refresh_item_activate_cb(GtkWidget *widget,
					gpointer data __attribute__((unused)))
{
	refresh_quickopen_menu(GTK_MENU(widget));
}

void
help_menu_manual_item_activate_cb(GtkWidget *widget __attribute__((unused)),
				  gpointer data __attribute__((unused)))
{
	gtk_show_uri(NULL, HELP_URI, GDK_CURRENT_TIME, NULL);
}

void
generic_quit_cb(GtkWidget *widget __attribute__((unused)),
		gpointer data __attribute__((unused)))
{
	gtk_main_quit();
}

static inline gboolean
quickopen_filter(const gchar *name)
{
	gchar *name_reversed = g_strreverse(g_strdup(name));
	gchar **filters, **filter;

	gchar *trans_name, *p;
	gboolean res;

	filters = g_strsplit(EXPERIMENT_MOVIE_FILTER, ";", 0);
	for (filter = filters; *filter != NULL; filter++) {
		GPatternSpec *pattern = g_pattern_spec_new(*filter);

		res = g_pattern_match(pattern, strlen(name), name, name_reversed);
		g_pattern_spec_free(pattern);
		if (res)
			break;
	}
	res = *filter == NULL;
	g_strfreev(filters);
	g_free(name_reversed);
	if (res)
		return FALSE;

	trans_name = g_strconcat(quickopen_directory, "/", name,
				 EXPERIMENT_TRANSCRIPT_EXT, NULL);
	if ((p = g_strrstr(trans_name, ".")) == NULL) {
		g_free(trans_name);
		return FALSE;
	}
	g_stpcpy(++p, EXPERIMENT_TRANSCRIPT_EXT);

	res = !g_access(trans_name, F_OK | R_OK);
	g_free(trans_name);

	return res;
}

static gint
quickopen_item_cmp(gconstpointer a, gconstpointer b)
{
	return -g_strcmp0(gtk_menu_item_get_label(*(GtkMenuItem **)a),
			  gtk_menu_item_get_label(*(GtkMenuItem **)b));
}

static void
refresh_quickopen_menu(GtkMenu *menu)
{
	static gchar		**fullnames = NULL;
	static GPtrArray	*items = NULL;

	int fullnames_n;

	GDir *dir;
	const gchar *name;

	if (items != NULL)
		g_ptr_array_free(items, TRUE);
	items = g_ptr_array_new_with_free_func((GDestroyNotify)gtk_widget_destroy);

	g_strfreev(fullnames);
	fullnames = NULL;
	fullnames_n = 0;

	dir = g_dir_open(quickopen_directory, 0, NULL);

	while ((name = g_dir_read_name(dir)) != NULL) {
		if (!quickopen_filter(name))
			continue;

		gchar *item_name, *p;
		GtkWidget *item;

		fullnames = g_realloc(fullnames, (fullnames_n+2) * sizeof(gchar *));
		fullnames[fullnames_n] = g_strconcat(quickopen_directory, "/",
						     name, NULL);

		item_name = g_strdup(name);
		if ((p = g_strrstr(item_name, ".")) != NULL)
			*p = '\0';
		item = gtk_check_menu_item_new_with_label(item_name);
		g_ptr_array_add(items, item);
		g_free(item_name);

		gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item),
						      TRUE);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
					       !g_strcmp0(current_filename,
					       	          fullnames[fullnames_n]));

		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(quickopen_item_on_activate),
				 fullnames[fullnames_n]);

		fullnames_n++;
	}
	if (fullnames != NULL)
		fullnames[fullnames_n] = NULL;
	g_ptr_array_sort(items, quickopen_item_cmp);

	g_dir_close(dir);

	for (int i = 0; i < items->len; i++) {
		GtkWidget *item = GTK_WIDGET(g_ptr_array_index(items, i));

		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		gtk_widget_show(item);
	}

	if (fullnames_n > 0)
		gtk_widget_hide(quickopen_menu_empty_item);
	else
		gtk_widget_show(quickopen_menu_empty_item);
}

static void
reconfigure_all_check_menu_items_cb(GtkWidget *widget, gpointer user_data)
{
	if (GTK_IS_CHECK_MENU_ITEM(widget))
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
					       widget == GTK_WIDGET(user_data));
}

static void
quickopen_item_on_activate(GtkWidget *widget, gpointer user_data)
{
	const gchar *filename = (const gchar *)user_data;

	gtk_container_foreach(GTK_CONTAINER(quickopen_menu),
			      reconfigure_all_check_menu_items_cb, widget);

	if (load_media_file(filename)) {
		/* FIXME */
	}
}

static inline void
button_image_set_from_stock(GtkButton *widget, const gchar *name)
{
	GtkWidget *image = gtk_bin_get_child(GTK_BIN(widget));

	gtk_image_set_from_stock(GTK_IMAGE(image), name,
				 GTK_ICON_SIZE_SMALL_TOOLBAR);
}

static gboolean
load_media_file(const gchar *uri)
{
	if (gtk_vlc_player_load(GTK_VLC_PLAYER(player_widget), uri))
		return TRUE;

	g_free(current_filename);
	current_filename = g_strdup(uri);

	gtk_widget_set_sensitive(controls_hbox, TRUE);

	return FALSE;
}

int
main(int argc, char *argv[])
{
	GtkBuilder *builder;
	GtkAdjustment *adj;

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

	gtk_widget_show_all(player_window);

	quickopen_directory = g_strdup(DEFAULT_QUICKOPEN_DIR);
	refresh_quickopen_menu(GTK_MENU(quickopen_menu));

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}