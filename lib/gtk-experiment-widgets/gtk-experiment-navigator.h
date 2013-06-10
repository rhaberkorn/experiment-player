/**
 * @file
 * Header file necessary to include when using the \e GtkExperimentNavigator
 * widget.
 */

/*
 * Copyright (C) 2012-2013 Otto-von-Guericke-Universität Magdeburg
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
 * @return \e obj casted to \e GtkExperimentNavigator
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

/** @private */
typedef struct _GtkExperimentNavigatorPrivate GtkExperimentNavigatorPrivate;

/**
 * \e GtkExperimentNavigator instance structure
 */
typedef struct _GtkExperimentNavigator {
	GtkTreeView parent_instance;		/**< Parent instance structure */

	/**
	 * @todo
	 * Add necessary \b public instance attributes. They must be
	 * initialized in the instance initializer function.
	 */

	GtkExperimentNavigatorPrivate *priv;	/**< @private Pointer to \b private instance attributes */
} GtkExperimentNavigator;

/**
 * \e GtkExperimentNavigator class structure
 */
typedef struct _GtkExperimentNavigatorClass {
	GtkTreeViewClass parent_class;	/**< Parent class structure */

	/**
	 * Callback function to invoke when emitting the "time-selected"
	 * signal.
	 *
	 * @param self          \e GtkExperimentNavigator the event was emitted on.
	 * @param selected_time Time selected by the navigator in milliseconds
	 */
	void (*time_selected)(GtkExperimentNavigator *self,
			      gint64 selected_time);

	/**
	 * Callback function to invoke when emitting the "section-activated"
	 * signal.
	 *
	 * @param self  \e GtkExperimentNavigator the event was emitted on.
	 * @param start Start time of section in milliseconds
	 * @param end   End time of section in milliseconds
	 */
	void (*section_activated)(GtkExperimentNavigator *self,
				  gint64 start, gint64 end);
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
