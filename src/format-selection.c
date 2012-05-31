#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gtk/gtk.h>

#include <gtk-experiment-transcript.h>

#include "experiment-player.h"

static void refresh_formats_store(GtkListStore *store);

GtkWidget *transcript_wizard_combo,
	  *transcript_proband_combo;

static gchar *formats_directory;

enum {
	COL_NAME,
	COL_FILENAME,
	NUM_COLS
};

/*
 * GtkBuilder signal callbacks
 * NOTE: for some strange reason the parameters are switched
 */

void
formats_menu_choosedir_item_activate_cb(GtkWidget *widget,
					gpointer data __attribute__((unused)))
{
	GtkTreeModel *model =
		gtk_combo_box_get_model(GTK_COMBO_BOX(transcript_wizard_combo));

	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Choose Directory...", GTK_WINDOW(widget),
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					     NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
					    formats_directory);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		g_free(formats_directory);
		formats_directory =
			gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		refresh_formats_store(GTK_LIST_STORE(model));
	}

	gtk_widget_destroy(dialog);
}

void
formats_menu_refresh_item_activate_cb(GtkWidget *widget,
				      gpointer data __attribute__((unused)))
{
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));

	refresh_formats_store(GTK_LIST_STORE(model));
}

void
generic_transcript_combo_changed_cb(gpointer user_data, GtkComboBox *combo)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(user_data);

	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	GtkTreeIter iter;
	gchar *filename = NULL;

	if (gtk_combo_box_get_active_iter(combo, &iter))
		gtk_tree_model_get(model, &iter, COL_FILENAME, &filename, -1);

	/*
	 * filename may be empty (null-entry) in which case any active format
	 * will be reset
	 */
	gtk_experiment_transcript_load_formats(trans, filename);
	g_free(filename);

#if 0
	refresh_formats_store(GTK_LIST_STORE(model));
#endif
}

void
generic_transcript_entry_changed_cb(gpointer user_data, GtkEditable *editable)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(user_data);
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

	/** @todo enable/disable markup */
	gtk_experiment_transcript_set_interactive_format(trans, text, TRUE);
}

static void
refresh_formats_store(GtkListStore *store)
{
	static GPatternSpec *pattern = NULL;

	GtkTreeModel *model = GTK_TREE_MODEL(store);
	gchar *wizard_filename = NULL;
	gchar *proband_filename = NULL;

	GDir *dir;
	const gchar *name;
	GtkTreeIter iter;

	if (pattern == NULL)
		pattern = g_pattern_spec_new(EXPERIMENT_FORMAT_FILTER);

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(transcript_wizard_combo),
					  &iter))
		gtk_tree_model_get(model, &iter,
				   COL_FILENAME, &wizard_filename, -1);
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(transcript_proband_combo),
					  &iter))
		gtk_tree_model_get(model, &iter,
				   COL_FILENAME, &proband_filename, -1);

	gtk_list_store_clear(store);
	/* add null-entry */
	gtk_list_store_append(store, &iter);

	dir = g_dir_open(formats_directory, 0, NULL);

	while ((name = g_dir_read_name(dir)) != NULL) {
		gchar *itemname, *fullname;

		if (!g_pattern_match_string(pattern, name))
			continue;

		itemname = path_strip_extension(name);
		fullname = g_build_filename(formats_directory, name, NULL);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   COL_NAME,		itemname,
				   COL_FILENAME,	fullname,
				   -1);

		if (!g_strcmp0(fullname, wizard_filename))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(transcript_wizard_combo),
						      &iter);
		if (!g_strcmp0(fullname, proband_filename))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(transcript_proband_combo),
						      &iter);

		g_free(fullname);
		g_free(itemname);
	}

	g_dir_close(dir);

	g_free(proband_filename);
	g_free(wizard_filename);
}

void
format_selection_init(const gchar *dir)
{
	GtkListStore	*formats_store;
	GtkCellRenderer	*renderer;

	formats_directory = g_strdup(dir);

	formats_store = gtk_list_store_new(NUM_COLS,
					   G_TYPE_STRING, G_TYPE_STRING);
	gtk_combo_box_set_model(GTK_COMBO_BOX(transcript_wizard_combo),
				GTK_TREE_MODEL(formats_store));
	gtk_combo_box_set_model(GTK_COMBO_BOX(transcript_proband_combo),
				GTK_TREE_MODEL(formats_store));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(transcript_wizard_combo),
				   renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(transcript_wizard_combo),
				      renderer, "text", COL_NAME);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(transcript_proband_combo),
				   renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(transcript_proband_combo),
				      renderer, "text", COL_NAME);

	refresh_formats_store(formats_store);

	g_object_unref(formats_store);
}
