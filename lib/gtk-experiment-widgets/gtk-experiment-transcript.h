/**
 * @file
 * Header file necessary to include when using the \e GtkExperimentTranscript
 * widget.
 */

/*
 * Copyright (C) 2012 Otto-von-Guericke-Universität Magdeburg
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

#ifndef __GTK_EXPERIMENT_TRANSCRIPT_H
#define __GTK_EXPERIMENT_TRANSCRIPT_H

#include <glib-object.h>
#include <glib.h>
#include <gdk/gdk.h>

#include <gtk/gtk.h>

#include <experiment-reader.h>

G_BEGIN_DECLS

/** \e GtkExperimentTranscript error domain */
#define GTK_EXPERIMENT_TRANSCRIPT_ERROR \
	(gtk_experiment_transcript_error_quark())

/** \e GtkExperimentTranscript error codes */
typedef enum {
	GTK_EXPERIMENT_TRANSCRIPT_ERROR_FILEOPEN,	/**< Error opening file */
	GTK_EXPERIMENT_TRANSCRIPT_ERROR_REGEXCAPTURES	/**< Additional regular expression captures used */
} GtkExperimentTranscriptError;

/** \e GtkExperimentTranscript type */
#define GTK_TYPE_EXPERIMENT_TRANSCRIPT \
	(gtk_experiment_transcript_get_type())
/**
 * Cast instance pointer to \e GtkExperimentTranscript
 *
 * @param obj Object to cast to \e GtkExperimentTranscript
 * @return \e obj casted to \e GtkExperimentTranscript
 */
#define GTK_EXPERIMENT_TRANSCRIPT(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_EXPERIMENT_TRANSCRIPT, GtkExperimentTranscript))
#define GTK_EXPERIMENT_TRANSCRIPT_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_EXPERIMENT_TRANSCRIPT, GtkExperimentTranscriptClass))
#define GTK_IS_EXPERIMENT_TRANSCRIPT(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_EXPERIMENT_TRANSCRIPT))
#define GTK_IS_EXPERIMENT_TRANSCRIPT_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_EXPERIMENT_TRANSCRIPT))
#define GTK_EXPERIMENT_TRANSCRIPT_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_EXPERIMENT_TRANSCRIPT, GtkExperimentTranscriptClass))

/** @private */
typedef struct _GtkExperimentTranscriptPrivate GtkExperimentTranscriptPrivate;

/**
 * \e GtkExperimentTranscript instance structure
 */
typedef struct _GtkExperimentTranscript {
	GtkWidget parent_instance;		/**< Parent instance structure */

	gchar *speaker;				/**< Name of speaker whose contributions are displayed (\b read-only) */

	/**
	 * Default formattings to apply for interactive for interactive format
	 * rules if no Pango markup is specified.
	 * A \c NULL pointer means that the correspondig text property will not
	 * be changed. After widget instantiation all fields are \c NULL.
	 */
	struct _GtkExperimentTranscriptInteractiveFormat {
		PangoFontDescription *default_font;	/**< Default interactive format font */
		GdkColor *default_text_color;		/**< Default interactive format text color */
		GdkColor *default_bg_color;		/**< Default interactive format background color */
	} interactive_format;

	GtkExperimentTranscriptPrivate *priv;	/**< @private Pointer to \b private instance attributes */
} GtkExperimentTranscript;

/**
 * \e GtkExperimentTranscript class structure
 */
typedef struct _GtkExperimentTranscriptClass {
	GtkWidgetClass parent_class;	/**< Parent class structure */
} GtkExperimentTranscriptClass;

/** @private */
GQuark gtk_experiment_transcript_error_quark(void);
/** @private */
GType gtk_experiment_transcript_get_type(void);

/*
 * API
 */
GtkWidget *gtk_experiment_transcript_new(const gchar *speaker);

gboolean gtk_experiment_transcript_load(GtkExperimentTranscript *trans,
					ExperimentReader *exp);
gboolean gtk_experiment_transcript_load_filename(GtkExperimentTranscript *trans,
						 const gchar *filename);

void gtk_experiment_transcript_set_use_backdrop_area(GtkExperimentTranscript *trans,
						     gboolean use);
gboolean gtk_experiment_transcript_get_use_backdrop_area(GtkExperimentTranscript *trans);
void gtk_experiment_transcript_set_backdrop_area(GtkExperimentTranscript *trans,
						 gint64 start, gint64 end);

void gtk_experiment_transcript_set_reverse_mode(GtkExperimentTranscript *trans,
						gboolean reverse);
gboolean gtk_experiment_transcript_get_reverse_mode(GtkExperimentTranscript *trans);

void gtk_experiment_transcript_set_alignment(GtkExperimentTranscript *trans,
					     PangoAlignment alignment);
PangoAlignment gtk_experiment_transcript_get_alignment(GtkExperimentTranscript *trans);

gboolean gtk_experiment_transcript_load_formats(GtkExperimentTranscript *trans,
						const gchar *filename,
						GError **error);
gboolean gtk_experiment_transcript_set_interactive_format(GtkExperimentTranscript *trans,
							  const gchar *format_str,
							  gboolean with_markup,
							  GError **error);

GtkAdjustment *gtk_experiment_transcript_get_time_adjustment(GtkExperimentTranscript *trans);
void gtk_experiment_transcript_set_time_adjustment(GtkExperimentTranscript *trans,
						   GtkAdjustment *adj);

G_END_DECLS

#endif
