#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <gtk/gtk.h>
#include <experiment-reader.h>

#include "gtk-experiment-navigator.h"

static void gtk_experiment_navigator_class_init(GtkExperimentNavigatorClass *klass);
static void gtk_experiment_navigator_init(GtkExperimentNavigator *klass);

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
	GtkTreeView *view = GTK_TREE_VIEW(klass);

	/* TODO */
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
