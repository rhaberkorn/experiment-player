#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gtk/gtk.h>
#include <experiment-reader.h>

#include "gtk-experiment-navigator.h"

static void gtk_experiment_navigator_class_init(GtkExperimentNavigatorClass *klass);
static void gtk_experiment_navigator_init(GtkExperimentNavigator *klass);

static void time_cell_data_cb(GtkTreeViewColumn *col,
			      GtkCellRenderer *renderer,
			      GtkTreeModel *model, GtkTreeIter *iter,
			      gpointer user_data);


static inline void select_time(GtkExperimentNavigator *navi, gint64 selected_time);

enum {
	TIME_SELECTED_SIGNAL,
	LAST_SIGNAL
};
static guint gtk_experiment_navigator_signals[LAST_SIGNAL] = {0};

enum {
	COL_SECTION_NAME,
	COL_TIME,
	NUM_COLS
};

GType
gtk_experiment_navigator_get_type(void)
{
	static GType type = 0;

	if (!type) {
		/* FIXME: destructor needed */
		const GTypeInfo info = {
			sizeof(GtkExperimentNavigatorClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)gtk_experiment_navigator_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(GtkExperimentNavigator),
			0,    /* n_preallocs */
			(GInstanceInitFunc)gtk_experiment_navigator_init,
		};

		type = g_type_register_static(GTK_TYPE_TREE_VIEW,
					      "GtkExperimentNavigator", &info, 0);
	}

	return type;
}

static void
gtk_experiment_navigator_class_init(GtkExperimentNavigatorClass *klass)
{
	gtk_experiment_navigator_signals[TIME_SELECTED_SIGNAL] =
		g_signal_new("time-selected",
			     G_TYPE_FROM_CLASS(klass),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GtkExperimentNavigatorClass, time_selected),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__LONG, G_TYPE_NONE,
			     1, G_TYPE_INT64);
}

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
	 */
	store = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_UINT64);

	/*
	 * Create some sample(!) store rows and content
	 */
	gtk_tree_store_append(store, &toplevel, NULL);
	gtk_tree_store_set(store, &toplevel,
			   COL_SECTION_NAME, "FOO", -1);

	gtk_tree_store_append(store, &child, &toplevel);
	gtk_tree_store_set(store, &child,
			   COL_SECTION_NAME, "BAR",
			   COL_TIME, 5*60*1000 /* 5 minutes */,
			   -1);

	/*
	 * Create TreeView column corresponding to the TreeStore column COL_SECTION_NAME
	 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(view, col);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_SECTION_NAME);

	/*
	 * Create TreeView column corresponding to the TreeStore column COL_TIME
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

	/* TODO */
}

static void
time_cell_data_cb(GtkTreeViewColumn *col __attribute__((unused)),
		  GtkCellRenderer *renderer,
		  GtkTreeModel *model, GtkTreeIter *iter,
		  gpointer user_data __attribute__((unused)))
{
	gint64	time_val;
	gchar	buf[20];

	gtk_tree_model_get(model, iter,
			   COL_TIME, &time_val, -1);

	g_snprintf(buf, sizeof(buf), "%lldms", time_val); /* FIXME */

	g_object_set(renderer, "text", buf, NULL);
}

static inline void
select_time(GtkExperimentNavigator *navi, gint64 selected_time)
{
	g_signal_emit(navi, gtk_experiment_navigator_signals[TIME_SELECTED_SIGNAL], 0,
		      selected_time);
}

/*
 * API
 */

GtkWidget *
gtk_experiment_navigator_new(void)
{
	return GTK_WIDGET(g_object_new(GTK_TYPE_EXPERIMENT_NAVIGATOR, NULL));
}

gboolean
gtk_experiment_navigator_load(GtkExperimentNavigator *navi,
			      ExperimentReader *exp)
{
	/* TODO */
	return TRUE;
}

gboolean
gtk_experiment_navigator_load_filename(GtkExperimentNavigator *navi,
				       const gchar *exp)
{
	/* TODO */
	return TRUE;
}
