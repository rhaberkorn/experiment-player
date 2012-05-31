/**
 * @file
 * Header file necessary to include when using the \e GtkExperimentTranscript
 * widget.
 */

#ifndef __GTK_EXPERIMENT_TRANSCRIPT_H
#define __GTK_EXPERIMENT_TRANSCRIPT_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include <experiment-reader.h>

G_BEGIN_DECLS

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
	gboolean reverse;			/**< Reverse mode (\b read-write) */

	GtkExperimentTranscriptPrivate *priv;	/**< @private Pointer to \b private instance attributes */
} GtkExperimentTranscript;

/**
 * \e GtkExperimentTranscript class structure
 */
typedef struct _GtkExperimentTranscriptClass {
	GtkWidgetClass parent_class;	/**< Parent class structure */
} GtkExperimentTranscriptClass;

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

gboolean gtk_experiment_transcript_load_formats(GtkExperimentTranscript *trans,
						const gchar *filename);
gboolean gtk_experiment_transcript_set_interactive_format(GtkExperimentTranscript *trans,
							  const gchar *format_str,
							  gboolean with_markup);

GtkAdjustment *gtk_experiment_transcript_get_time_adjustment(GtkExperimentTranscript *trans);
void gtk_experiment_transcript_set_time_adjustment(GtkExperimentTranscript *trans,
						   GtkAdjustment *adj);

G_END_DECLS

#endif
