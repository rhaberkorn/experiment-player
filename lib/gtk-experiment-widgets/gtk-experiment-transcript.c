/**
 * @file
 * GTK widget, extending a \e GtkWidget, for displaying an experiment's
 * transcript.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>
#include <experiment-reader.h>

#include "gtk-experiment-transcript-private.h"

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

static gboolean button_pressed(GtkWidget *widget, GdkEventButton *event);
static void choose_font_activated(GtkWidget *widget, gpointer data);
static void choose_fg_color_activated(GtkWidget *widget, gpointer data);
static void choose_bg_color_activated(GtkWidget *widget, gpointer data);
static void reverse_activated(GtkWidget *widget, gpointer data);

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

	widget_class->button_press_event = button_pressed;

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
	GtkWidget *item;

	klass->priv = GTK_EXPERIMENT_TRANSCRIPT_GET_PRIVATE(klass);

	klass->speaker = NULL;
	klass->reverse = FALSE;

	klass->interactive_format.default_font = NULL;
	klass->interactive_format.default_text_color = NULL;
	klass->interactive_format.default_bg_color = NULL;

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
	klass->priv->formats = NULL;
	klass->priv->interactive_format.regexp = NULL;
	klass->priv->interactive_format.attribs = NULL;

	klass->priv->menu = gtk_menu_new();
	gtk_menu_attach_to_widget(GTK_MENU(klass->priv->menu),
				  GTK_WIDGET(klass), NULL);

	item = gtk_menu_item_new_with_mnemonic("_Choose Font...");
	g_signal_connect(item, "activate",
			 G_CALLBACK(choose_font_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	item = gtk_menu_item_new_with_mnemonic("Choose _Foreground Color...");
	g_signal_connect(item, "activate",
			 G_CALLBACK(choose_fg_color_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	item = gtk_menu_item_new_with_mnemonic("Choose _Background Color...");
	g_signal_connect(item, "activate",
			 G_CALLBACK(choose_bg_color_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	item = gtk_check_menu_item_new_with_mnemonic("_Reverse");
	g_signal_connect(item, "activate",
			 G_CALLBACK(reverse_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);
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

	pango_font_description_free(trans->interactive_format.default_font);
	gdk_color_free(trans->interactive_format.default_text_color);
	gdk_color_free(trans->interactive_format.default_bg_color);

	experiment_reader_free_contributions(trans->priv->contribs);
	gtk_experiment_transcript_free_formats(trans->priv->formats);
	gtk_experiment_transcript_free_format(&trans->priv->interactive_format);

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
			      | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
	attributes.visual = gtk_widget_get_visual(widget);
	attributes.colormap = gtk_widget_get_colormap(widget);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
					&attributes, attributes_mask);

	widget->style = gtk_style_attach(widget->style, widget->window);

	gdk_window_set_user_data(widget->window, widget);

	gtk_style_set_background(widget->style, widget->window, GTK_STATE_ACTIVE);

	/* FIXME */
	gtk_experiment_transcript_configure(widget, NULL);
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

	gtk_experiment_transcript_text_layer_redraw(trans);

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
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(user_data);

	/**
	 * @todo
	 * heuristic to improve performance in the common case of advancing
	 * time in small steps
	 */
	gtk_experiment_transcript_text_layer_redraw(trans);
}

/** @private */
void
gtk_experiment_transcript_text_layer_redraw(GtkExperimentTranscript *trans)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	gint64 current_time = 0;
	gint last_contrib_y;

	gdk_draw_rectangle(GDK_DRAWABLE(trans->priv->layer_text),
			   widget->style->bg_gc[gtk_widget_get_state(widget)],
			   TRUE,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height + LAYER_TEXT_INVISIBLE);

	gtk_widget_queue_draw_area(widget, 0, 0,
				   widget->allocation.width,
				   widget->allocation.height);

	if (trans->priv->contribs == NULL)
		return;

	/** @todo reverse mode */

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

		PangoAttrList *attrib_list;

		if (y > widget->allocation.height + LAYER_TEXT_INVISIBLE)
			continue;

		attrib_list = pango_attr_list_new();

		for (GSList *cur = trans->priv->formats; cur != NULL; cur = cur->next) {
			GtkExperimentTranscriptFormat *fmt =
				(GtkExperimentTranscriptFormat *)cur->data;

			gtk_experiment_transcript_apply_format(fmt, contrib->text,
							       attrib_list);
		}
		gtk_experiment_transcript_apply_format(&trans->priv->interactive_format,
						       contrib->text, attrib_list);

		pango_layout_set_attributes(trans->priv->layer_text_layout,
					    attrib_list);
		pango_attr_list_unref(attrib_list);

		pango_layout_set_text(trans->priv->layer_text_layout,
				      contrib->text, -1);
		pango_layout_set_height(trans->priv->layer_text_layout,
					(last_contrib_y - y)*PANGO_SCALE);

		pango_layout_get_pixel_size(trans->priv->layer_text_layout,
					    NULL, &logical_height);
		if (y + logical_height < 0)
			break;

		gdk_draw_layout(GDK_DRAWABLE(trans->priv->layer_text),
				widget->style->text_gc[gtk_widget_get_state(widget)],
				0, y, trans->priv->layer_text_layout);
		last_contrib_y = y;
	}
}

static gboolean
button_pressed(GtkWidget *widget, GdkEventButton *event)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(widget);

	/* Ignore double-clicks and triple-clicks */
	if (event->button != 3 || event->type != GDK_BUTTON_PRESS)
		return FALSE;

	gtk_menu_popup(GTK_MENU(trans->priv->menu), NULL, NULL, NULL, NULL,
                       event->button, event->time);
	return TRUE;
}

static void
choose_font_activated(GtkWidget *widget __attribute__((unused)),
		      gpointer data)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(data);

	GtkWidget *dialog;
	gchar *font_name;

	dialog = gtk_font_selection_dialog_new("Choose Font...");

	font_name = pango_font_description_to_string(GTK_WIDGET(trans)->style->font_desc);
	gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog),
						font_name);
	g_free(font_name);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		PangoFontDescription *font_desc;

		font_name = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));
		font_desc = pango_font_description_from_string(font_name);

		gtk_widget_modify_font(GTK_WIDGET(trans), font_desc);

		pango_font_description_free(font_desc);
		g_free(font_name);

		gtk_experiment_transcript_text_layer_redraw(trans);
	}

	gtk_widget_destroy(dialog);
}

static void
choose_fg_color_activated(GtkWidget *widget __attribute__((unused)),
			  gpointer data)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(data);

	GtkWidget *dialog, *colorsel;

	dialog = gtk_color_selection_dialog_new("Choose Foreground Color...");
	colorsel = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dialog));

	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(colorsel),
					      &GTK_WIDGET(trans)->style->text[GTK_STATE_NORMAL]);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		GdkColor color;

		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(colorsel),
						      &color);
		gtk_widget_modify_text(GTK_WIDGET(trans),
				       GTK_STATE_NORMAL, &color);

		gtk_experiment_transcript_text_layer_redraw(trans);
	}

	gtk_widget_destroy(dialog);
}

static void
choose_bg_color_activated(GtkWidget *widget __attribute__((unused)),
			  gpointer data)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(data);

	GtkWidget *dialog, *colorsel;

	dialog = gtk_color_selection_dialog_new("Choose Background Color...");
	colorsel = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dialog));

	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(colorsel),
					      &GTK_WIDGET(trans)->style->bg[GTK_STATE_NORMAL]);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		GdkColor color;

		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(colorsel),
						      &color);
		gtk_widget_modify_bg(GTK_WIDGET(trans),
				     GTK_STATE_NORMAL, &color);

		gtk_experiment_transcript_text_layer_redraw(trans);
	}

	gtk_widget_destroy(dialog);
}

static void
reverse_activated(GtkWidget *widget, gpointer data)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(data);

	trans->reverse =
		gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
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

	gtk_experiment_transcript_text_layer_redraw(trans);

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
