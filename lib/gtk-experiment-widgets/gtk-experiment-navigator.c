/**
 * @file
 * GTK widget, extending a \e GtkTreeView, for displaying an experiment's
 * structure for navigational purposes.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <inttypes.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gtk/gtk.h>
#include <experiment-reader.h>

#include "cclosure-marshallers.h"
#include "gtk-experiment-navigator.h"

static void gtk_experiment_navigator_class_init(GtkExperimentNavigatorClass *klass);
static void gtk_experiment_navigator_init(GtkExperimentNavigator *klass);

static void time_cell_data_cb(GtkTreeViewColumn *col,
			      GtkCellRenderer *renderer,
			      GtkTreeModel *model, GtkTreeIter *iter,
			      gpointer user_data);


static inline void select_time(GtkExperimentNavigator *navi, gint64 selected_time);

/** @private */
enum {
	TIME_SELECTED_SIGNAL,
	LAST_SIGNAL
};
static guint gtk_experiment_navigator_signals[LAST_SIGNAL] = {0};

/**
 * @private
 * Enumeration of tree store columns that serve as Ids when manipulating
 * the store.
 */
enum {
	COL_NAME,	/**< Name of the section, subsection or topic (\e G_TYPE_STRING) */
	COL_START_TIME,	/**< Start time of the entity (\e G_TYPE_UINT64 in milliseconds) */
	NUM_COLS	/**< Number of columns */
};

/**
 * @private
 * will create gtk_experiment_navigator_get_type and set
 * gtk_experiment_navigator_parent_class
 */
G_DEFINE_TYPE(GtkExperimentNavigator, gtk_experiment_navigator, GTK_TYPE_TREE_VIEW);

static void
gtk_experiment_navigator_class_init(GtkExperimentNavigatorClass *klass)
{
	gtk_experiment_navigator_signals[TIME_SELECTED_SIGNAL] =
		g_signal_new("time-selected",
			     G_TYPE_FROM_CLASS(klass),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GtkExperimentNavigatorClass, time_selected),
			     NULL, NULL,
			     gtk_experiment_navigator_marshal_VOID__INT64,
			     G_TYPE_NONE, 1, G_TYPE_INT64);
}

/**
 * Instance initializer for the \e GtkExperimentNavigator widget.
 * It has to create the \e GtkTreeStore (MVC model), add and configure
 * view columns and add cell renderers to the view columns.
 * It should connect the necessary signals to respond to row activations
 * (double click) in order to emit the "time-selected" signal.
 *
 * @param klass Newly constructed \e GtkExperimentNavigator instance
 */
static void
gtk_experiment_navigator_init(GtkExperimentNavigator *klass)
{
	GtkTreeView		*view = GTK_TREE_VIEW(klass);
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer;

	GtkTreeStore	*store;
	GtkTreeIter	toplevel, child;

	/*
	 * Create tree store (and model)
	 * NOTE: GtkTreeStore is directly derived from GObject and has a
	 * reference count of 1 after creation.
	 */
	store = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_UINT64);

	/*
	 * Create some sample(!) store rows and content
	 */
	/** @todo remove sample code */
	gtk_tree_store_append(store, &toplevel, NULL);
	gtk_tree_store_set(store, &toplevel,
			   COL_NAME,		"greeting",
			   -1);

	gtk_tree_store_append(store, &child, &toplevel);
	gtk_tree_store_set(store, &child,
			   COL_NAME,		"bz_1",
			   COL_START_TIME,	(gint64)5*60*1000 /* 5 minutes */,
			   -1);

	/*
	 * Create TreeView column corresponding to the
	 * TreeStore column \e COL_NAME
	 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(view, col);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_NAME);
	/**
	 * @todo Perhaps an icon should be rendered in front of the name to
	 *       indicate the entity's type (section, subsection, topic...)
	 */

	/*
	 * Create TreeView column corresponding to the
	 * TreeStore column \e COL_START_TIME
	 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Time");
	gtk_tree_view_append_column(view, col);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	/* Cell data function for custom formatting */
	gtk_tree_view_column_set_cell_data_func(col, renderer, time_cell_data_cb,
						NULL, NULL);

	/*
	 * Set TreeView model to store's model
	 */
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));
	/* destroy store/model automatically with view */
	g_object_unref(store);

	/** @todo better \e TreeViewColumn formatting */
	/** @todo connect signals to respond to row activations */
}

/**
 * Cell data function to invoke when rendering the "Time" column.
 *
 * @param col       \e GtkTreeViewColumn to render for
 * @param renderer  Cell renderer to use for rendering
 * @param model     \e GtkTreeModel associated with the view
 * @param iter      Row identifier
 * @param user_data Callback user data
 */
static void
time_cell_data_cb(GtkTreeViewColumn *col __attribute__((unused)),
		  GtkCellRenderer *renderer,
		  GtkTreeModel *model, GtkTreeIter *iter,
		  gpointer user_data __attribute__((unused)))
{
	gint64	time_val;
	gchar	buf[20];

	gtk_tree_model_get(model, iter,
			   COL_START_TIME, &time_val, -1);

	/** @todo Improve readability (e.g. h:mm:ss) */
	/** @todo Do we always want to render the time column??? */
	g_snprintf(buf, sizeof(buf), "%" PRId64 "ms", time_val);

	g_object_set(renderer, "text", buf, NULL);
}

/**
 * Emit "time-selected" signal on a \e GtkExperimentNavigator instance.
 * It should be emitted when a row entry was selected.
 *
 * @param navi          Widget to emit the signal
 * @param selected_time Selected time in milliseconds
 */
static inline void
select_time(GtkExperimentNavigator *navi, gint64 selected_time)
{
	g_signal_emit(navi, gtk_experiment_navigator_signals[TIME_SELECTED_SIGNAL], 0,
		      selected_time);
}

/*
 * API
 */

/**
 * Construct new \e GtkExperimentNavigator widget instance.
 *
 * @return New \e GtkExperimentNavigator widget instance
 */
GtkWidget *
gtk_experiment_navigator_new(void)
{
	return GTK_WIDGET(g_object_new(GTK_TYPE_EXPERIMENT_NAVIGATOR, NULL));
}

/**
 * Fills the \e GtkExperimentNavigator widget with the structure specified
 * in an experiment-XML file (see session.dtd).
 * Any existing contents should be cleared.
 *
 * @param navi Object instance to display the structure in
 * @param exp  \e ExperimentReader instance symbolizing the XML-file
 * @return \e TRUE on error, else \e FALSE
 */
gboolean
gtk_experiment_navigator_load(GtkExperimentNavigator *navi,
			      ExperimentReader *exp)
{
	/** @todo Clear contents, process XML file and fill \e TreeViewStore */

	return TRUE;
}

/**
 * Fills the \e GtkExperimentNavigator widget with the structure specified
 * in an experiment-XML file (see session.dtd).
 * Any existing contents should be cleared.
 *
 * @param navi Object instance to display the structure in
 * @param exp  Filename of XML-file to open and use for configuring \e navi
 * @return \e TRUE on error, else \e FALSE
 */
gboolean
gtk_experiment_navigator_load_filename(GtkExperimentNavigator *navi,
				       const gchar *exp)
{
	/** @todo Clear contents, process XML file and fill \e TreeViewStore */

	return TRUE;
}
