/**
 * @file
 * Private header for the \e GtkExperimentTranscript widget
 */

#ifndef __GTK_EXPERIMENT_TRANSCRIPT_PRIVATE_H
#define __GTK_EXPERIMENT_TRANSCRIPT_PRIVATE_H

#include "gtk-experiment-transcript.h"

/** @private */
#define GTK_EXPERIMENT_TRANSCRIPT_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_EXPERIMENT_TRANSCRIPT, GtkExperimentTranscriptPrivate))

/** @private */
typedef struct _GtkExperimentTranscriptFormat {
	GRegex		*regexp;
	PangoAttrList	*attribs;
} GtkExperimentTranscriptFormat;

/**
 * @private
 * Private instance attribute structure.
 * You can access these attributes using \c klass->priv->attribute.
 */
struct _GtkExperimentTranscriptPrivate {
	GtkObject	*time_adjustment;
	gulong		time_adj_on_value_changed_id;

	GdkPixmap	*layer_text;
	PangoLayout	*layer_text_layout;

	GList		*contribs;
	GSList		*formats;
	GtkExperimentTranscriptFormat interactive_format;

	GtkWidget	*menu;			/**< Drop-down menu, doesn't have to be unreferenced manually */
	GSList		*alignment_group;	/**< GtkRadioMenuItem group (owned by GTK) */
};

#define DEFAULT_WIDTH		100
#define DEFAULT_HEIGHT		200

#define LAYER_TEXT_INVISIBLE	100

/** @todo scale should be configurable */
#define PX_PER_SECOND		15
#define TIME_TO_PX(TIME)	((TIME)/(1000/PX_PER_SECOND))
#define PX_TO_TIME(PX)		(((PX)*1000)/PX_PER_SECOND)

/**
 * @private
 * Unreference object given by variable, but only once.
 * Use it in \ref gtk_experiment_transcript_dispose to unreference object
 * references in public or private instance attributes.
 *
 * @sa gtk_experiment_transcript_dispose
 *
 * @param VAR Variable to unreference
 */
#define GOBJECT_UNREF_SAFE(VAR) do {	\
	if ((VAR) != NULL) {		\
		g_object_unref(VAR);	\
		VAR = NULL;		\
	}				\
} while (0)

/** @private */
void gtk_experiment_transcript_text_layer_redraw(GtkExperimentTranscript *trans);

/** @private */
void gtk_experiment_transcript_apply_format(GtkExperimentTranscriptFormat *fmt,
					    const gchar *text,
					    PangoAttrList *attrib_list);

/** @private */
static inline void
gtk_experiment_transcript_free_format(GtkExperimentTranscriptFormat *format)
{
	if (format->regexp != NULL)
		g_regex_unref(format->regexp);
	if (format->attribs != NULL)
		pango_attr_list_unref(format->attribs);
}

/** @private */
void gtk_experiment_transcript_free_formats(GSList *formats);



#endif
