/**
 * @file
 * Header file necessary to include when using the \e GtkExperimentNavigator
 * widget.
 */

#ifndef __GTK_EXPERIMENT_NAVIGATOR_H
#define __GTK_EXPERIMENT_NAVIGATOR_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include <experiment-reader.h>

G_BEGIN_DECLS

#define GTK_TYPE_EXPERIMENT_NAVIGATOR \
	(gtk_experiment_navigator_get_type())
/**
 * Cast instance pointer to \e GtkExperimentNavigator
 *
 * @param obj Object to cast to \e GtkExperimentNavigator
 */
#define GTK_EXPERIMENT_NAVIGATOR(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_EXPERIMENT_NAVIGATOR, GtkExperimentNavigator))
#define GTK_EXPERIMENT_NAVIGATOR_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_EXPERIMENT_NAVIGATOR, GtkExperimentNavigatorClass))
#define GTK_IS_EXPERIMENT_NAVIGATOR(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_EXPERIMENT_NAVIGATOR))
#define GTK_IS_EXPERIMENT_NAVIGATOR_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_EXPERIMENT_NAVIGATOR))
#define GTK_EXPERIMENT_NAVIGATOR_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_EXPERIMENT_NAVIGATOR, GtkExperimentNavigatorClass))

/**
 * \e GtkExperimentNavigator instance structure
 */
typedef struct _GtkExperimentNavigator {
	GtkTreeView parent_instance;	/**< Parent instance structure */

	/** @todo Add necessary instance attributes */
} GtkExperimentNavigator;

/**
 * \e GtkExperimentNavigator class structure
 */
typedef struct _GtkExperimentNavigatorClass {
	GtkTreeViewClass parent_class;	/**< Parent class structure */

	/**
	 * Callback function to invoke when emitting the "time-selected"
	 * signal. Do not set manually.
	 */
	void (*time_selected)(GtkExperimentNavigator *self,
			      gint64 selected_time, gpointer user_data);
} GtkExperimentNavigatorClass;

/** @private */
GType gtk_experiment_navigator_get_type(void);

/*
 * API
 */
GtkWidget *gtk_experiment_navigator_new(void);

gboolean gtk_experiment_navigator_load(GtkExperimentNavigator *navi,
				       ExperimentReader *exp);
gboolean gtk_experiment_navigator_load_filename(GtkExperimentNavigator *navi,
						const gchar *exp);

G_END_DECLS

#endif
