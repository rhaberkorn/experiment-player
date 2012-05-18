/**
 * @file
 * GTK widget, extending a \e GtkWidget, for displaying an experiment's
 * transcript.
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
#include "gtk-experiment-transcript.h"

static void gtk_experiment_transcript_class_init(GtkExperimentTranscriptClass *klass);
static void gtk_experiment_transcript_init(GtkExperimentTranscript *klass);

static void gtk_experiment_transcript_realize(GtkWidget *widget);
static void gtk_experiment_transcript_size_request(GtkWidget *widget,
						   GtkRequisition *requisition);
static void gtk_experiment_transcript_size_allocate(GtkWidget *widget,
						    GtkAllocation *allocation);
static gboolean gtk_experiment_transcript_configure(GtkWidget *widget,
						    GdkEventConfigure *event);
static gboolean gtk_experiment_transcript_expose(GtkWidget *widget,
						 GdkEventExpose *event);

static void gtk_experiment_transcript_dispose(GObject *gobject);
static void gtk_experiment_transcript_finalize(GObject *gobject);

static void time_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data);

static void text_layer_redraw(GtkExperimentTranscript *trans);

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
#define GTK_EXPERIMENT_TRANSCRIPT_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_EXPERIMENT_TRANSCRIPT, GtkExperimentTranscriptPrivate))

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
};

/**
 * @private
 * Will create \e gtk_experiment_transcript_get_type and set
 * \e gtk_experiment_transcript_parent_class
 */
G_DEFINE_TYPE(GtkExperimentTranscript, gtk_experiment_transcript, GTK_TYPE_WIDGET);

static void
gtk_experiment_transcript_class_init(GtkExperimentTranscriptClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gobject_class->dispose = gtk_experiment_transcript_dispose;
	gobject_class->finalize = gtk_experiment_transcript_finalize;

	widget_class->realize = gtk_experiment_transcript_realize;
	/* FIXME: configure-event handler not invoked! */
	widget_class->configure_event = gtk_experiment_transcript_configure;
	widget_class->expose_event = gtk_experiment_transcript_expose;
	widget_class->size_request = gtk_experiment_transcript_size_request;
	widget_class->size_allocate = gtk_experiment_transcript_size_allocate;

	g_type_class_add_private(klass, sizeof(GtkExperimentTranscriptPrivate));
}

/**
 * @brief Instance initializer function for the \e GtkExperimentTranscript widget
 *
 * @param klass Newly constructed \e GtkExperimentTranscript instance
 */
static void
gtk_experiment_transcript_init(GtkExperimentTranscript *klass)
{
	klass->priv = GTK_EXPERIMENT_TRANSCRIPT_GET_PRIVATE(klass);

	klass->speaker = NULL;

	klass->priv->time_adjustment = gtk_adjustment_new(0., 0., 0.,
							  0., 0., 0.);
	g_object_ref_sink(klass->priv->time_adjustment);
	klass->priv->time_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(klass->priv->time_adjustment),
				 "value-changed",
				 G_CALLBACK(time_adj_on_value_changed), klass);

	klass->priv->layer_text = NULL;
	klass->priv->layer_text_layout =
		gtk_widget_create_pango_layout(GTK_WIDGET(klass), NULL);
	pango_layout_set_wrap(klass->priv->layer_text_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize(klass->priv->layer_text_layout, PANGO_ELLIPSIZE_END);

	klass->priv->contribs = NULL;
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
 * @sa gtk_experiment_transcript_finalize
 * @sa gtk_experiment_transcript_init
 *
 * @param gobject \e GObject to dispose
 */
static void
gtk_experiment_transcript_dispose(GObject *gobject)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(gobject);

	/*
	 * destroy might be called more than once, but we have only one
	 * reference for each object
	 */
	if (trans->priv->time_adjustment != NULL) {
		g_signal_handler_disconnect(G_OBJECT(trans->priv->time_adjustment),
					    trans->priv->time_adj_on_value_changed_id);
		g_object_unref(trans->priv->time_adjustment);
		trans->priv->time_adjustment = NULL;
	}
	GOBJECT_UNREF_SAFE(trans->priv->layer_text);
	GOBJECT_UNREF_SAFE(trans->priv->layer_text_layout);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(gtk_experiment_transcript_parent_class)->dispose(gobject);
}

/**
 * @brief Instance finalization function
 *
 * Its purpose is to free all remaining allocated memory referenced
 * in public and private instance attributes (e.g. a string).
 * For freeing (unreferencing) objects,
 * use \ref gtk_experiment_transcript_dispose.
 *
 * @sa gtk_experiment_transcript_dispose
 * @sa gtk_experiment_transcript_init
 *
 * @param gobject \e GObject to finalize
 */
static void
gtk_experiment_transcript_finalize(GObject *gobject)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(gobject);

	g_free(trans->speaker);
	experiment_reader_free_contributions(trans->priv->contribs);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(gtk_experiment_transcript_parent_class)->finalize(gobject);
}

static void
gtk_experiment_transcript_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;

	gtk_widget_set_realized(widget, TRUE);

	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = gtk_widget_get_events(widget)
			      | GDK_EXPOSURE_MASK;
	attributes.visual = gtk_widget_get_visual(widget);
	attributes.colormap = gtk_widget_get_colormap(widget);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
					&attributes, attributes_mask);

	widget->style = gtk_style_attach(widget->style, widget->window);

	gdk_window_set_user_data(widget->window, widget);

	gtk_style_set_background(widget->style, widget->window, GTK_STATE_ACTIVE);
}

static void 
gtk_experiment_transcript_size_request(GtkWidget *widget __attribute__((unused)),
				       GtkRequisition *requisition)
{
	requisition->width = DEFAULT_WIDTH;
	requisition->height = DEFAULT_HEIGHT;
}

static void
gtk_experiment_transcript_size_allocate(GtkWidget *widget,
					GtkAllocation *allocation)
{
	widget->allocation = *allocation;

	if (gtk_widget_get_realized(widget)) {
		gdk_window_move_resize(gtk_widget_get_window(widget),
				       allocation->x, allocation->y,
				       allocation->width, allocation->height);

		/* FIXME */
		gtk_experiment_transcript_configure(widget, NULL);
	}
}

static gboolean
gtk_experiment_transcript_configure(GtkWidget *widget,
				    GdkEventConfigure *event __attribute__((unused)))
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(widget);

	gtk_adjustment_set_page_size(GTK_ADJUSTMENT(trans->priv->time_adjustment),
				     (gdouble)PX_TO_TIME(widget->allocation.height));

	GOBJECT_UNREF_SAFE(trans->priv->layer_text);
	trans->priv->layer_text = gdk_pixmap_new(gtk_widget_get_window(widget),
						 widget->allocation.width,
						 widget->allocation.height + LAYER_TEXT_INVISIBLE,
						 -1);
	pango_layout_set_width(trans->priv->layer_text_layout,
			       widget->allocation.width*PANGO_SCALE);

	text_layer_redraw(trans);

	return TRUE;
}

static gboolean
gtk_experiment_transcript_expose(GtkWidget *widget, GdkEventExpose *event)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(widget);

	gdk_draw_drawable(GDK_DRAWABLE(gtk_widget_get_window(widget)),
			  widget->style->fg_gc[gtk_widget_get_state(widget)],
			  GDK_DRAWABLE(trans->priv->layer_text),
			  event->area.x, event->area.y,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);

	return FALSE;
}

static void
time_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data)
{
	/**
	 * @todo
	 * heuristic to improve performance in the common case of advancing
	 * time in small steps
	 */
	text_layer_redraw(GTK_EXPERIMENT_TRANSCRIPT(user_data));
}

static void
text_layer_redraw(GtkExperimentTranscript *trans)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	gint64 current_time = 0;
	gint last_contrib_y;

	gdk_draw_rectangle(GDK_DRAWABLE(trans->priv->layer_text),
			   widget->style->white_gc,
			   TRUE,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height + LAYER_TEXT_INVISIBLE);

	gtk_widget_queue_draw_area(widget, 0, 0,
				   widget->allocation.width,
				   widget->allocation.height);

	if (trans->priv->contribs == NULL)
		return;

	if (trans->priv->time_adjustment != NULL)
		current_time = (gint64)gtk_adjustment_get_value(GTK_ADJUSTMENT(trans->priv->time_adjustment));
	last_contrib_y = widget->allocation.height + LAYER_TEXT_INVISIBLE;

	for (GList *cur = experiment_reader_get_contribution_by_time(trans->priv->contribs,
								     current_time);
	     cur != NULL;
	     cur = cur->prev) {
		ExperimentReaderContrib *contrib = (ExperimentReaderContrib *)cur->data;

		gint y = widget->allocation.height -
			 TIME_TO_PX(current_time - contrib->start_time);
		int logical_height;

		if (y > widget->allocation.height + LAYER_TEXT_INVISIBLE)
			continue;

		/** @todo add attributes according to regexp masks and search mask */
		/* does that reset default attributes for the widget? */
		pango_layout_set_attributes(trans->priv->layer_text_layout, NULL);

		pango_layout_set_text(trans->priv->layer_text_layout,
				      contrib->text, -1);
		pango_layout_set_height(trans->priv->layer_text_layout,
					(last_contrib_y - y)*PANGO_SCALE);

		pango_layout_get_pixel_size(trans->priv->layer_text_layout,
					    NULL, &logical_height);
		if (y + logical_height < 0)
			break;

		gdk_draw_layout(GDK_DRAWABLE(trans->priv->layer_text),
				widget->style->black_gc,
				0, y, trans->priv->layer_text_layout);
		last_contrib_y = y;
	}
}

/*
 * API
 */

/**
 * @brief Construct new \e GtkExperimentTranscript widget instance.
 *
 * @param speaker Name of speaker whose contributions are displayed
 * @return New \e GtkExperimentTranscript widget instance
 */
GtkWidget *
gtk_experiment_transcript_new(const gchar *speaker)
{
	GtkExperimentTranscript *trans;

	trans = GTK_EXPERIMENT_TRANSCRIPT(g_object_new(GTK_TYPE_EXPERIMENT_TRANSCRIPT, NULL));
	trans->speaker = g_strdup(speaker);
	if (trans->speaker == NULL) {
		gtk_widget_destroy(GTK_WIDGET(trans));
		return NULL;
	}

	return GTK_WIDGET(trans);
}

gboolean
gtk_experiment_transcript_load(GtkExperimentTranscript *trans,
			       ExperimentReader *exp)
{
	experiment_reader_free_contributions(trans->priv->contribs);
	trans->priv->contribs =
		experiment_reader_get_contributions_by_speaker(exp, trans->speaker);

	text_layer_redraw(trans);

	return trans->priv->contribs == NULL;
}

gboolean
gtk_experiment_transcript_load_filename(GtkExperimentTranscript *trans,
					const gchar *filename)
{
	gboolean res = TRUE;
	ExperimentReader *exp = experiment_reader_new(filename);

	if (exp != NULL) {
		res = gtk_experiment_transcript_load(trans, exp);
		g_object_unref(exp);
	}

	return res;
}

GtkAdjustment *
gtk_experiment_transcript_get_time_adjustment(GtkExperimentTranscript *trans)
{
	return trans->priv->time_adjustment != NULL
			? GTK_ADJUSTMENT(trans->priv->time_adjustment)
			: NULL;
}

/**
 * @brief Change time-adjustment used by \e GtkExperimentTranscript
 *
 * The old adjustment will be
 * unreferenced (and possibly destroyed) and a reference to the new
 * adjustment will be fetched.
 *
 * @sa gtk_experiment_transcript_get_time_adjustment
 *
 * @param trans \e GtkExperimentTranscript instance
 * @param adj   New \e GtkAdjustment to use as time-adjustment.
 */
void
gtk_experiment_transcript_set_time_adjustment(GtkExperimentTranscript *trans,
					      GtkAdjustment *adj)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	if (trans->priv->time_adjustment == NULL)
		return;

	g_signal_handler_disconnect(G_OBJECT(trans->priv->time_adjustment),
				    trans->priv->time_adj_on_value_changed_id);

	g_object_unref(trans->priv->time_adjustment);
	trans->priv->time_adjustment = GTK_OBJECT(adj);
	g_object_ref_sink(trans->priv->time_adjustment);

	gtk_adjustment_set_page_size(GTK_ADJUSTMENT(trans->priv->time_adjustment),
				     (gdouble)PX_TO_TIME(widget->allocation.height));

	trans->priv->time_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(trans->priv->time_adjustment),
				 "value-changed",
				 G_CALLBACK(time_adj_on_value_changed), trans);
}
