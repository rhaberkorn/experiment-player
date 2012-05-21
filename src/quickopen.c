#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "experiment-player.h"

static inline gboolean quickopen_filter(const gchar *name);
static gint quickopen_item_cmp(gconstpointer a, gconstpointer b);

static void reconfigure_all_check_menu_items_cb(GtkWidget *widget, gpointer user_data);
static void quickopen_item_on_activate(GtkWidget *widget, gpointer user_data);

GtkWidget *quickopen_menu,
	  *quickopen_menu_empty_item;

gchar *quickopen_directory;

/*
 * GtkBuilder signal callbacks
 * NOTE: for some strange reason the parameters are switched
 */

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

/** @todo refresh item unnecessary, rebuild menu when it is opened */
void
quickopen_menu_refresh_item_activate_cb(GtkWidget *widget,
					gpointer data __attribute__((unused)))
{
	refresh_quickopen_menu(GTK_MENU(widget));
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

	trans_name = g_build_filename(quickopen_directory, name, NULL);
	trans_name = g_realloc(trans_name, strlen(trans_name) +
					   sizeof(EXPERIMENT_TRANSCRIPT_EXT));
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

void
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
		fullnames[fullnames_n] = g_build_filename(quickopen_directory,
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

	gtk_widget_set_visible(quickopen_menu_empty_item, !fullnames_n);
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
	gchar *trans_name, *p;

	gtk_container_foreach(GTK_CONTAINER(quickopen_menu),
			      reconfigure_all_check_menu_items_cb, widget);

	if (load_media_file(filename)) {
		/* FIXME */
	}

	trans_name = g_strdup(filename);
	trans_name = g_realloc(trans_name, strlen(trans_name) +
					   sizeof(EXPERIMENT_TRANSCRIPT_EXT));
	if ((p = g_strrstr(trans_name, ".")) == NULL) {
		g_free(trans_name);
		/* FIXME */
		return;
	}
	g_stpcpy(++p, EXPERIMENT_TRANSCRIPT_EXT);

	if (load_transcript_file(trans_name)) {
		/* FIXME */
	}

	g_free(trans_name);
}
