/**
 * @file
 * GTK widget, extending a \e GtkTreeView, for displaying an experiment's
 * structure for navigational purposes.
 */

/*
 * Copyright (C) 2012 Otto-von-Guericke-Universit√§t Magdeburg
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gtk/gtk.h>
#include <experiment-reader.h>

#include "cclosure-marshallers.h"
#include "gtk-experiment-navigator.h"

static void gtk_experiment_navigator_class_init(GtkExperimentNavigatorClass *klass);
static void gtk_experiment_navigator_init(GtkExperimentNavigator *klass);

static void gtk_experiment_navigator_dispose(GObject *gobject);
static void gtk_experiment_navigator_finalize(GObject *gobject);

static void gtk_experiment_navigator_row_activated(GtkTreeView *tree_view,
						   GtkTreePath *path,
						   GtkTreeViewColumn *column);
static void gtk_experiment_navigator_cursor_changed(GtkTreeView *tree_view);

static void time_cell_data_cb(GtkTreeViewColumn *col,
			      GtkCellRenderer *renderer,
			      GtkTreeModel *model, GtkTreeIter *iter,
			      gpointer user_data);

static inline void select_time(GtkExperimentNavigator *navi,
			       gint64 selected_time);
static inline void activate_section(GtkExperimentNavigator *navi,
				    gint64 start, gint64 end);

static void topic_row_callback(ExperimentReader *reader,
			const gchar *topic_id,
			gint64 start_time,
			gint64 end_time,
			gpointer data);

/**
 * @private
 * Unreference object given by variable, but only once.
 * Use it in \ref gtk_experiment_navigator_dispose to unreference object
 * references in public or private instance attributes.
 *
 * @sa gtk_experiment_navigator_dispose
 *
 * @param VAR Variable to unreference
 */
#define GOBJECT_UNREF_SAFE(VAR) G_STMT_START {	\
	if ((VAR) != NULL) {			\
		g_object_unref(VAR);		\
		VAR = NULL;			\
	}					\
} G_STMT_END

/** @private */
#define GTK_EXPERIMENT_NAVIGATOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_EXPERIMENT_NAVIGATOR, GtkExperimentNavigatorPrivate))

/**
 * @private
 * Private instance attribute structure.
 * You can access these attributes using \c klass->priv->attribute.
 */
struct _GtkExperimentNavigatorPrivate {
	gint dummy; /**< unused dummy attribute, may be deleted when other attributes are added */

	/**
	 * @todo
	 * Add necessary \b private instance attributes. They must be
	 * initialized in the instance initializer function.
	 */
};

struct TopicCallbackData {
	GtkTreeIter iter;
	GtkTreeStore *store;
	gint64 start_time;
	gint64 end_time;
};

/** @private */
enum {
	TIME_SELECTED_SIGNAL,
	SECTION_ACTIVATED_SIGNAL,
	LAST_SIGNAL
};
static guint gtk_experiment_navigator_signals[LAST_SIGNAL] = {0};

/**
 * @private
 * Enumeration of tree store columns that serve as Ids when manipulating
 * the store.
 */
enum {
	COL_NAME,	/**< Name of the section, subsection or topic (\c G_TYPE_STRING) */
	COL_START_TIME,	/**< Start time of the entity (\c G_TYPE_INT64 in milliseconds) */
	COL_END_TIME,	/**< End time of the entity (\c G_TYPE_INT64 in milliseconds) */
	NUM_COLS	/**< Number of columns */

	/** @todo Add additional tree store columns as necessary */
};

/**
 * @private
 * Will create \e gtk_experiment_navigator_get_type and set
 * \e gtk_experiment_navigator_parent_class
 */
G_DEFINE_TYPE(GtkExperimentNavigator, gtk_experiment_navigator, GTK_TYPE_TREE_VIEW);

static void
gtk_experiment_navigator_class_init(GtkExperimentNavigatorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkTreeViewClass *treeview_class = GTK_TREE_VIEW_CLASS(klass);

	gobject_class->dispose = gtk_experiment_navigator_dispose;
	gobject_class->finalize = gtk_experiment_navigator_finalize;

	treeview_class->row_activated = gtk_experiment_navigator_row_activated;
	treeview_class->cursor_changed = gtk_experiment_navigator_cursor_changed;

	gtk_experiment_navigator_signals[TIME_SELECTED_SIGNAL] =
		g_signal_new("time-selected",
			     G_TYPE_FROM_CLASS(klass),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GtkExperimentNavigatorClass, time_selected),
			     NULL, NULL,
			     gtk_experiment_widgets_marshal_VOID__INT64,
			     G_TYPE_NONE, 1, G_TYPE_INT64);

	gtk_experiment_navigator_signals[SECTION_ACTIVATED_SIGNAL] =
		g_signal_new("section-activated",
			     G_TYPE_FROM_CLASS(klass),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GtkExperimentNavigatorClass, section_activated),
			     NULL, NULL,
			     gtk_experiment_widgets_marshal_VOID__INT64_INT64,
			     G_TYPE_NONE, 2, G_TYPE_INT64, G_TYPE_INT64);

	g_type_class_add_private(klass, sizeof(GtkExperimentNavigatorPrivate));
}

/**
 * @brief Instance initializer function for the \e GtkExperimentNavigator widget
 *
 * It has to create the \e GtkTreeStore (MVC model), add and configure
 * view columns and add cell renderers to the view columns.
 * It should connect the necessary signals to respond to row activations
 * (double click) in order to emit the "time-selected" signal.
 * It should also initialize all used \b public and \b private attributes.
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

	klass->priv = GTK_EXPERIMENT_NAVIGATOR_GET_PRIVATE(klass);
	/*
	 * Create tree store (and model)
	 * NOTE: GtkTreeStore is directly derived from GObject and has a
	 * reference count of 1 after creation.
	 */
	store = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING,
				   G_TYPE_INT64, G_TYPE_INT64);

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
	 * @todo
	 * Perhaps an icon should be rendered in front of the name to
	 * indicate the entity's type (section, subsection, topic...)
	 */

	/*
	 * Create TreeView column corresponding to the
	 * TreeStore column \c COL_START_TIME
	 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Start");
	gtk_tree_view_append_column(view, col);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	/* Cell data function for custom formatting */
	gtk_tree_view_column_set_cell_data_func(col, renderer,
						time_cell_data_cb,
						GINT_TO_POINTER(COL_START_TIME),
						NULL);

	/*
	 * Create TreeView column corresponding to the
	 * TreeStore column \c COL_END_TIME
	 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "End");
	gtk_tree_view_append_column(view, col);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	/* Cell data function for custom formatting */
	gtk_tree_view_column_set_cell_data_func(col, renderer,
						time_cell_data_cb,
						GINT_TO_POINTER(COL_END_TIME),
						NULL);

	/*
	 * Set TreeView model to store's model
	 */
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));
	/* destroy store/model automatically with view */
	g_object_unref(store);

	/** @todo better \e TreeViewColumn formatting */
	/**
	 * @todo
	 * Initialize necessary \b public and \b private attributes.
	 * When using object references, they must be unreferenced in
	 * \ref gtk_experiment_navigator_dispose.
	 * Keep in mind that when using objects derived from \e GtkObject,
	 * they will not be reference-counted like ordinary \e GObjects (you
	 * will not own an ordinary reference after object creation that can
	 * be unreferenced).
	 * In order to get an ordinary reference, use \e g_object_ref_sink
	 * on the object after creation.
	 */
}

/**
 * @brief Instance disposal function
 *
 * Its purpose is to unreference \e GObjects
 * the instance keeps references to (object pointers saved in the
 * instance attributes).
 * Keep in mind that this function may be called more than once, so
 * you must guard against unreferencing an object more than once (since you
 * will only own a single reference).
 * Also keep in mind that instance methods may be invoked \b after the instance
 * disposal function was executed but \b before instance finalization. This
 * case has to be handled gracefully in the instance methods.
 *
 * @sa GOBJECT_UNREF_SAFE
 * @sa gtk_experiment_navigator_finalize
 * @sa gtk_experiment_navigator_init
 *
 * @param gobject \e GObject to dispose
 */
static void
gtk_experiment_navigator_dispose(GObject *gobject)
{
	GtkExperimentNavigator *navi = GTK_EXPERIMENT_NAVIGATOR(gobject);

	/*
	 * destroy might be called more than once, but we have only one
	 * reference for each object
	 */
	/**
	 * @todo
	 * Unreference all \e GObject references kept in public or
	 * private attributes. Use \ref GOBJECT_UNREF_SAFE.
	 * For example, to unreference private object \c widget:
	 * @code
	 * 	GOBJECT_UNREF_SAFE(navi->priv->widget);
	 * @endcode
	 */

	/* Chain up to the parent class */
	G_OBJECT_CLASS(gtk_experiment_navigator_parent_class)->dispose(gobject);
}

/**
 * @brief Instance finalization function
 *
 * Its purpose is to free all remaining allocated memory referenced
 * in public and private instance attributes (e.g. a string).
 * For freeing (unreferencing) objects,
 * use \ref gtk_experiment_navigator_dispose.
 *
 * @sa gtk_experiment_navigator_dispose
 * @sa gtk_experiment_navigator_init
 *
 * @param gobject \e GObject to finalize
 */
static void
gtk_experiment_navigator_finalize(GObject *gobject)
{
	GtkExperimentNavigator *navi = GTK_EXPERIMENT_NAVIGATOR(gobject);

	/** @todo Free all memory referenced in public and private attributes */

	/* Chain up to the parent class */
	G_OBJECT_CLASS(gtk_experiment_navigator_parent_class)->finalize(gobject);
}

static void
gtk_experiment_navigator_row_activated(GtkTreeView *tree_view,
				       GtkTreePath *path,
				       GtkTreeViewColumn *column __attribute__((unused)))
{
	GtkTreeModel *treemodel = gtk_tree_view_get_model(tree_view);
	gint64 start_time;
	GtkTreeIter treeiter;

	gtk_tree_model_get_iter(treemodel, &treeiter, path);
	gtk_tree_model_get(treemodel, &treeiter,
			   COL_START_TIME, &start_time,
			   -1);

	select_time(GTK_EXPERIMENT_NAVIGATOR(tree_view), start_time);
}

static void
gtk_experiment_navigator_cursor_changed(GtkTreeView *tree_view)
{
	GtkTreeModel *treemodel = gtk_tree_view_get_model(tree_view);
	GtkTreePath *treepath;
	GtkTreeIter treeiter;
	gint64 start_time;
	gint64 end_time;

	gtk_tree_view_get_cursor(tree_view, &treepath, NULL);
	gtk_tree_model_get_iter(treemodel, &treeiter, treepath);
	gtk_tree_path_free(treepath);

	gtk_tree_model_get(treemodel, &treeiter,
			   COL_START_TIME, &start_time,
			   COL_END_TIME, &end_time,
			   -1);

	activate_section(GTK_EXPERIMENT_NAVIGATOR(tree_view), start_time, end_time);
}

/**
 * @brief Cell data function to invoke when rendering the "Start" and
 *        "End" columns.
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
		  gpointer user_data)
{
	gint column = GPOINTER_TO_INT(user_data);

	gint64	time_val;
	gchar	buf[20];

	gtk_tree_model_get(model, iter,
			   column, &time_val, -1);

	g_snprintf(buf, sizeof(buf), "%" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT ,
				time_val /1000/60, time_val/1000 % 60);

	g_object_set(renderer, "text", buf, NULL);
}

/**
 * @brief Emit "time-selected" signal on a \e GtkExperimentNavigator instance.
 *
 * It should be emitted when a row entry was selected (double-clicked).
 *
 * @sa GtkExperimentNavigatorClass::time_selected
 *
 * @param navi          Widget to emit the signal on
 * @param selected_time Selected time in milliseconds
 */
static inline void
select_time(GtkExperimentNavigator *navi, gint64 selected_time)
{
	g_signal_emit(navi, gtk_experiment_navigator_signals[TIME_SELECTED_SIGNAL], 0,
		      selected_time);
}

/**
 * @brief Emit "section-activated" signal on a \e GtkExperimentNavigator instance.
 *
 * It should be emitted when a row entry was activated (e.g. single-clicked)
 *
 * @sa GtkExperimentNavigatorClass::section_activated
 *
 * @param navi  Widget to emit the signal on
 * @param start Start time of section in milliseconds
 * @param end   End time of section in milliseconds
 */
static inline void
activate_section(GtkExperimentNavigator *navi, gint64 start, gint64 end)
{
	g_signal_emit(navi, gtk_experiment_navigator_signals[SECTION_ACTIVATED_SIGNAL], 0,
		      start, end);
}

/**
 * Callback - legt neue Row an und f¸gt diese in sotre ein
 * ¸bergibt Spaltenname und Startzeit aus Userdata
 * @todo: ¸bersetzen
 */
static void
topic_row_callback(ExperimentReader *reader,
		   const gchar *topic_id,
		   gint64 start_time,
		   gint64 end_time,
		   gpointer data)
{
	struct TopicCallbackData *tcb = (struct TopicCallbackData *) data;
	GtkTreeIter topic;

	if (tcb->start_time < 0)
		tcb->start_time = start_time;
	tcb->end_time = end_time;

	gtk_tree_store_append(tcb->store, &topic, &tcb->iter);
	gtk_tree_store_set(tcb->store, &topic,
			   COL_NAME, topic_id,
			   COL_START_TIME, start_time,
			   COL_END_TIME, end_time,
			   -1);
}

/*
 * API
 */

/**
 * @brief Construct new \e GtkExperimentNavigator widget instance.
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
 * @param exp  \e ExperimentReader instance of opened XML-file
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_experiment_navigator_load(GtkExperimentNavigator *navi,
			      ExperimentReader *exp)
{
	struct TopicCallbackData tcd;
	GtkTreeIter experiment_level;
	GtkTreeIter last_minute_level;

	tcd.store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(navi)));
	gtk_tree_store_clear(tcd.store);

	/* greeting */
	gtk_tree_store_append(tcd.store, &tcd.iter, NULL);

	tcd.start_time = -1;
	experiment_reader_foreach_greeting_topic(exp, topic_row_callback, &tcd);

	gtk_tree_store_set(tcd.store, &tcd.iter,
			   COL_NAME,		"greeting",
			   COL_START_TIME,	tcd.start_time,
			   COL_END_TIME,	tcd.end_time,
			   -1);

	/* experiment */
	gtk_tree_store_append(tcd.store, &experiment_level, NULL);
	gtk_tree_store_set(tcd.store, &experiment_level,
			   COL_NAME,		"experiment",
			   -1);

	gtk_tree_store_append(tcd.store, &tcd.iter, &experiment_level);

	tcd.start_time = -1;
	experiment_reader_foreach_exp_initial_narrative_topic(exp,
							      topic_row_callback,
							      &tcd);

	gtk_tree_store_set(tcd.store, &tcd.iter,
			   COL_NAME,		"initial-narrative",
			   COL_START_TIME,	tcd.start_time < 0 ? tcd.end_time
							           : tcd.start_time,
			   COL_END_TIME,	tcd.end_time,
			   -1);

	gtk_tree_store_set(tcd.store, &experiment_level,
			   COL_START_TIME, tcd.start_time < 0 ? tcd.end_time
							      : tcd.start_time,
			   -1);

	gtk_tree_store_append(tcd.store, &last_minute_level, &experiment_level);
	gtk_tree_store_set(tcd.store, &last_minute_level,
			   COL_NAME, "last minute",
			   -1);

	for (gint i = 1; i <= 6; i++) {
		gchar phasename[8];

		g_snprintf(phasename, sizeof(phasename), "phase %d", i);
		gtk_tree_store_append(tcd.store,
				      &tcd.iter,
				      &last_minute_level);

		tcd.start_time = -1;
		experiment_reader_foreach_exp_last_minute_phase_topic(exp, i, topic_row_callback, &tcd);

		gtk_tree_store_set(tcd.store, &tcd.iter,
				   COL_NAME, phasename,
				   COL_START_TIME, tcd.start_time < 0 ? tcd.end_time
								      : tcd.start_time,
				   COL_END_TIME, tcd.end_time,
				   -1);

		if (i == 1) {
			gtk_tree_store_set(tcd.store, &last_minute_level,
					   COL_START_TIME,
					   tcd.start_time < 0 ? tcd.end_time
							      : tcd.start_time,
					   -1);
		}
	}

	gtk_tree_store_set(tcd.store, &last_minute_level,
			   COL_END_TIME, tcd.end_time,
			   -1);

	gtk_tree_store_set(tcd.store, &experiment_level,
			   COL_END_TIME, tcd.end_time,
			   -1);

	/* farewell */
	gtk_tree_store_append(tcd.store, &tcd.iter, NULL);

	tcd.start_time = -1;
	experiment_reader_foreach_farewell_topic(exp, topic_row_callback, &tcd);

	gtk_tree_store_set(tcd.store, &tcd.iter,
			   COL_NAME,		"farewell",
			   COL_START_TIME,	tcd.start_time,
			   COL_END_TIME,	tcd.end_time,
			   -1);

	return TRUE;
}

/**
 * Fills the \e GtkExperimentNavigator widget with the structure specified
 * in an experiment-XML file (see session.dtd).
 * It accepts an XML filename and is otherwise identical to
 * \ref gtk_experiment_navigator_load.
 *
 * @sa gtk_experiment_navigator_load
 *
 * @param navi Object instance to display the structure in
 * @param exp  Filename of XML-file to open and use for configuring \e navi
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_experiment_navigator_load_filename(GtkExperimentNavigator *navi,
				       const gchar *exp)
{
	gboolean returnvalue;
	ExperimentReader *expread = experiment_reader_new(exp);
	
	if (expread == NULL)
		return FALSE;
	returnvalue = gtk_experiment_navigator_load(navi, expread);
	
	g_object_unref(expread);
	
	return returnvalue;
}
