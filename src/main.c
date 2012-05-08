#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#include <gtk/gtk.h>
#include <gtk-vlc-player.h>

static int quickopen_filter_cb(const struct dirent *name);
static void destroy_all_check_menu_items_cb(GtkWidget *widget, gpointer data);
static void refresh_quickopen_menu(GtkMenu *menu);
static void reconfigure_all_check_menu_items_cb(GtkWidget *widget, gpointer user_data);
static void quickopen_item_on_activate(GtkWidget *widget, gpointer user_data);
static gboolean load_media_file(const gchar *uri);

#define BUILDER_INIT(BUILDER, VAR) do {					\
	VAR = GTK_WIDGET(gtk_builder_get_object(BUILDER, #VAR));	\
} while (0)

/* shortcut OR (S1 ? : S2) might not always work under windows */
#define SOR(S1, S2) \
	((S1) != NULL ? (S1) : (S2))

static GtkWidget *player_window;

static GtkWidget *player_widget,
		 *scale_widget,
		 *volume_button;

static GtkWidget *quickopen_menu,
		 *quickopen_menu_empty_item;

static char *current_filename = NULL;
static gchar *quickopen_directory;

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
generic_quit_cb(GtkWidget *widget __attribute__((unused)),
		gpointer data __attribute__((unused)))
{
	gtk_main_quit();
}

static int
quickopen_filter_cb(const struct dirent *name)
{
	char *filters, *filter;
	char *trans_name, *p;
	struct stat info;

	int rc;

	filters = strdup(EXPERIMENT_MOVIE_FILTER);
	for (filter = strtok_r(filters, ";", &p);
	     filter != NULL && fnmatch(filter, name->d_name, 0);
	     filter = strtok_r(NULL, ";", &p));
	free(filters);
	if (filter == NULL)
		return 0;

	trans_name = malloc(strlen(quickopen_directory) + 1 + strlen(name->d_name) +
			    sizeof(EXPERIMENT_TRANSCRIPT_EXT));
	strcpy(trans_name, quickopen_directory);
	strcat(trans_name, "/");
	strcat(trans_name, name->d_name);
	if ((p = strrchr(trans_name, '.')) == NULL) {
		free(trans_name);
		return 0;
	}
	strcpy(++p, EXPERIMENT_TRANSCRIPT_EXT);

	rc = !stat(trans_name, &info) && S_ISREG(info.st_mode);
	free(trans_name);
	return rc;
}

static void
destroy_all_check_menu_items_cb(GtkWidget *widget,
				gpointer data __attribute__((unused)))
{
	if (GTK_IS_CHECK_MENU_ITEM(widget))
		gtk_widget_destroy(widget);
}

static void
refresh_quickopen_menu(GtkMenu *menu)
{
	struct dirent **namelist;
	int n;

	static char **fullnames = NULL;

	gtk_container_foreach(GTK_CONTAINER(menu),
			      destroy_all_check_menu_items_cb, NULL);

	if (fullnames != NULL) {
		for (char **cur = fullnames; *cur != NULL; cur++)
			free(*cur);
		free(fullnames);
	}

	n = scandir(quickopen_directory, &namelist,
		    quickopen_filter_cb, alphasort);
	if (n < 0)
		return;

	if (n > 0)
		gtk_widget_hide(quickopen_menu_empty_item);
	else
		gtk_widget_show(quickopen_menu_empty_item);

	fullnames = malloc((n+1) * sizeof(char *));
	fullnames[n] = NULL;

	while (n--) {
		char *item_name = strdup(namelist[n]->d_name);
		char *p;

		GtkWidget *item;

		fullnames[n] = malloc(strlen(quickopen_directory) + 1 +
				      strlen(namelist[n]->d_name) + 1);
		strcpy(fullnames[n], quickopen_directory);
		strcat(fullnames[n], "/");
		strcat(fullnames[n], namelist[n]->d_name);

		if ((p = strrchr(item_name, '.')) != NULL)
			*p = '\0';

		item = gtk_check_menu_item_new_with_label((const gchar *)item_name);
		gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
					       !strcmp(SOR(current_filename, ""),
					       	       fullnames[n]));

		free(item_name);

		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(quickopen_item_on_activate), fullnames[n]);

		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		gtk_widget_show(item);

		free(namelist[n]);
	}
	free(namelist);
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

static gboolean
load_media_file(const gchar *uri)
{
	if (gtk_vlc_player_load(GTK_VLC_PLAYER(player_widget), uri))
		return TRUE;

	free(current_filename);
	current_filename = strdup(uri);

	gtk_widget_set_sensitive(scale_widget, TRUE);

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
	BUILDER_INIT(builder, scale_widget);
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

	quickopen_directory = strdup(DEFAULT_QUICKOPEN_DIR);
	refresh_quickopen_menu(GTK_MENU(quickopen_menu));

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}