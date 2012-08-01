/**
 * @file
 * GTK widget, extending a \e GtkWidget, for displaying an experiment's
 * transcript.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <experiment-reader.h>

#include "gtk-experiment-transcript.h"
#include "gtk-experiment-transcript-private.h"

static void gtk_experiment_transcript_class_init(GtkExperimentTranscriptClass *klass);
static void gtk_experiment_transcript_init(GtkExperimentTranscript *klass);

static void gtk_experiment_transcript_realize(GtkWidget *widget);
static void gtk_experiment_transcript_size_allocate(GtkWidget *widget,
						    GtkAllocation *allocation);
static gboolean gtk_experiment_transcript_expose(GtkWidget *widget,
						 GdkEventExpose *event);

static void gtk_experiment_transcript_dispose(GObject *gobject);
static void gtk_experiment_transcript_finalize(GObject *gobject);

static void time_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data);

static void gtk_experiment_transcript_reconfigure(GtkExperimentTranscript *trans);

static gboolean configure_text_layout(GtkExperimentTranscript *trans,
				      ExperimentReaderContrib *contrib,
				      gint64 current_time,
				      gint y, gint last_contrib_y,
				      int *logical_height);
static gboolean render_contribution_bottomup(GtkExperimentTranscript *trans,
					     ExperimentReaderContrib *contrib,
					     gint64 current_time, gint64 current_time_px,
					     gint *last_contrib_y);
static gboolean render_contribution_topdown(GtkExperimentTranscript *trans,
					    ExperimentReaderContrib *contrib,
					    gint64 current_time, gint64 current_time_px,
					    gint *last_contrib_y);
static inline void render_backdrop_area(GtkExperimentTranscript *trans,
					gint64 current_time_px);

static void state_changed(GtkWidget *widget, GtkStateType state);
static gboolean button_pressed(GtkWidget *widget, GdkEventButton *event);
static gboolean scrolled(GtkWidget *widget, GdkEventScroll *event);

static void choose_font_activated(GtkWidget *widget, gpointer data);
static void choose_text_color_activated(GtkWidget *widget, gpointer data);
static void choose_bg_color_activated(GtkWidget *widget, gpointer data);
static void text_alignment_activated(GtkWidget *widget, gpointer data);

static void reverse_activated(GtkWidget *widget, gpointer data);

/** @private */
GQuark
gtk_experiment_transcript_error_quark(void)
{
	return g_quark_from_static_string("gtk-experiment-transcript-error-quark");
}

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
	widget_class->expose_event = gtk_experiment_transcript_expose;
	widget_class->size_allocate = gtk_experiment_transcript_size_allocate;

	widget_class->state_changed = state_changed;
	widget_class->button_press_event = button_pressed;
	widget_class->scroll_event = scrolled;

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
	GtkWidget *item, *submenu, *image;

	klass->priv = GTK_EXPERIMENT_TRANSCRIPT_GET_PRIVATE(klass);

	klass->speaker = NULL;

	klass->interactive_format.default_font = NULL;
	klass->interactive_format.default_text_color = NULL;
	klass->interactive_format.default_bg_color = NULL;

	klass->priv->flag_mask = 0;

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

	klass->priv->backdrop.start = 0;
	klass->priv->backdrop.end = 0;

	klass->priv->contribs = NULL;
	klass->priv->formats = NULL;
	klass->priv->interactive_format.regexp = NULL;
	klass->priv->interactive_format.attribs = NULL;

	/** @todo It should be possible to reset font and colors (to widget defaults) */
	klass->priv->menu = gtk_menu_new();
	gtk_menu_attach_to_widget(GTK_MENU(klass->priv->menu),
				  GTK_WIDGET(klass), NULL);

	item = gtk_image_menu_item_new_with_mnemonic("Choose _Font...");
	image = gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT,
					 GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	g_signal_connect(item, "activate",
			 G_CALLBACK(choose_font_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	item = gtk_image_menu_item_new_with_mnemonic("Choose _Text Color...");
	image = gtk_image_new_from_stock(GTK_STOCK_SELECT_COLOR,
					 GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	g_signal_connect(item, "activate",
			 G_CALLBACK(choose_text_color_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	item = gtk_image_menu_item_new_with_mnemonic("Choose _Background Color...");
	image = gtk_image_new_from_stock(GTK_STOCK_SELECT_COLOR,
					 GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	g_signal_connect(item, "activate",
			 G_CALLBACK(choose_bg_color_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	submenu = gtk_menu_new();
	item = gtk_menu_item_new_with_label("Text Alignment");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	/*
	 * position in alignment_group list corresponds with PangoAlignment
	 * (PANGO_ALIGN_RIGHT, PANGO_ALIGN_CENTER, PANGO_ALIGN_LEFT)
	 */
	item = gtk_radio_menu_item_new_with_mnemonic(NULL, "_Left");
	klass->priv->alignment_group =
			gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	g_signal_connect(item, "activate",
			 G_CALLBACK(text_alignment_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	item = gtk_radio_menu_item_new_with_mnemonic(klass->priv->alignment_group,
						     "_Center");
	klass->priv->alignment_group =
			gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	g_signal_connect(item, "activate",
			 G_CALLBACK(text_alignment_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	item = gtk_radio_menu_item_new_with_mnemonic(klass->priv->alignment_group,
						     "_Right");
	klass->priv->alignment_group =
			gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	g_signal_connect(item, "activate",
			 G_CALLBACK(text_alignment_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	gtk_widget_show_all(submenu);
	gtk_experiment_transcript_set_alignment(klass, PANGO_ALIGN_LEFT);

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu), item);
	gtk_widget_show(item);

	klass->priv->menu_reverse_item =
			gtk_check_menu_item_new_with_mnemonic("_Reverse");
	g_signal_connect(klass->priv->menu_reverse_item, "activate",
			 G_CALLBACK(reverse_activated), klass);
	gtk_menu_shell_append(GTK_MENU_SHELL(klass->priv->menu),
			      klass->priv->menu_reverse_item);
	gtk_widget_show(klass->priv->menu_reverse_item);

	gtk_widget_set_can_focus(GTK_WIDGET(klass), TRUE);
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

	gtk_widget_set_has_window(widget, TRUE);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
					&attributes, attributes_mask);

	widget->style = gtk_style_attach(widget->style, widget->window);

	gdk_window_set_user_data(widget->window, widget);

	gtk_style_set_background(widget->style, widget->window, GTK_STATE_ACTIVE);

	gtk_experiment_transcript_reconfigure(GTK_EXPERIMENT_TRANSCRIPT(widget));
}

static void
gtk_experiment_transcript_size_allocate(GtkWidget *widget,
					GtkAllocation *allocation)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(widget);
	gboolean sizeChanged = widget->allocation.width != allocation->width ||
			       widget->allocation.height != allocation->height;

	gtk_widget_set_allocation(widget, allocation);

	if (gtk_widget_get_realized(widget)) {
		gdk_window_move_resize(gtk_widget_get_window(widget),
				       allocation->x, allocation->y,
				       allocation->width, allocation->height);

		if (sizeChanged || trans->priv->layer_text == NULL)
			gtk_experiment_transcript_reconfigure(trans);
	}
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

static void
gtk_experiment_transcript_reconfigure(GtkExperimentTranscript *trans)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	gtk_adjustment_set_page_size(GTK_ADJUSTMENT(trans->priv->time_adjustment),
				     (gdouble)PX_TO_TIME(widget->allocation.height));

	GOBJECT_UNREF_SAFE(trans->priv->layer_text);
	trans->priv->layer_text = gdk_pixmap_new(gtk_widget_get_window(widget),
						 widget->allocation.width,
						 widget->allocation.height,
						 -1);
	pango_layout_set_width(trans->priv->layer_text_layout,
			       widget->allocation.width*PANGO_SCALE);

	gtk_experiment_transcript_text_layer_redraw(trans);
}

static gboolean
configure_text_layout(GtkExperimentTranscript *trans,
		      ExperimentReaderContrib *contrib,
		      gint64 current_time,
		      gint y, gint last_contrib_y,
		      int *logical_height)
{
	PangoAttrList *attrib_list;

	if (contrib->start_time > current_time)
		return FALSE;

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
				last_contrib_y == -1
					? G_MAXINT
					: ABS(last_contrib_y - y)*PANGO_SCALE);

	pango_layout_get_pixel_size(trans->priv->layer_text_layout,
				    NULL, logical_height);

	return TRUE;
}

static gboolean
render_contribution_bottomup(GtkExperimentTranscript *trans,
			     ExperimentReaderContrib *contrib,
			     gint64 current_time, gint64 current_time_px,
			     gint *last_contrib_y)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	gint old_last_contrib_y = *last_contrib_y;
	int logical_height;

	*last_contrib_y = widget->allocation.height -
			  (current_time_px - TIME_TO_PX(contrib->start_time));

	if (!configure_text_layout(trans, contrib, current_time,
				   *last_contrib_y, old_last_contrib_y,
				   &logical_height))
		return TRUE;

	if (*last_contrib_y + logical_height < 0)
		return FALSE;

	gdk_draw_layout(GDK_DRAWABLE(trans->priv->layer_text),
			widget->style->text_gc[gtk_widget_get_state(widget)],
			0, *last_contrib_y,
			trans->priv->layer_text_layout);

	return *last_contrib_y > 0;
}

static gboolean
render_contribution_topdown(GtkExperimentTranscript *trans,
			    ExperimentReaderContrib *contrib,
			    gint64 current_time, gint64 current_time_px,
			    gint *last_contrib_y)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	gint old_last_contrib_y = *last_contrib_y;
	int logical_height;

	*last_contrib_y = current_time_px - TIME_TO_PX(contrib->start_time);

	if (!configure_text_layout(trans, contrib, current_time,
				   *last_contrib_y, old_last_contrib_y,
				   &logical_height))
		return TRUE;

	if (*last_contrib_y - logical_height > widget->allocation.height)
		return FALSE;

	gdk_draw_layout(GDK_DRAWABLE(trans->priv->layer_text),
			widget->style->text_gc[gtk_widget_get_state(widget)],
			0, *last_contrib_y - logical_height,
			trans->priv->layer_text_layout);

	return *last_contrib_y < widget->allocation.height;
}

static inline void
render_backdrop_area(GtkExperimentTranscript *trans, gint64 current_time_px)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	gint y_start, y_end;
	GdkColor color;
	GdkColor *bg = &widget->style->bg[gtk_widget_get_state(widget)];

	if (!gtk_experiment_transcript_get_use_backdrop_area(trans))
		return;

	if (gtk_experiment_transcript_get_reverse_mode(trans)) {
		y_end = current_time_px - TIME_TO_PX(trans->priv->backdrop.start);
		y_start = current_time_px - TIME_TO_PX(trans->priv->backdrop.end);
	} else {
		y_start = widget->allocation.height -
			  (current_time_px - TIME_TO_PX(trans->priv->backdrop.start));
		y_end = widget->allocation.height -
			(current_time_px - TIME_TO_PX(trans->priv->backdrop.end));
	}

	if ((y_start < 0 && y_end < 0) ||
	    (y_start > widget->allocation.height &&
	     y_end > widget->allocation.height))
		return;

	y_start = CLAMP(y_start, 0, widget->allocation.height);
	y_end = CLAMP(y_end, 0, widget->allocation.height);

	color.pixel = 0;
	color.red = MAX((gint)bg->red - BACKDROP_VALUE, 0);
	color.blue = MAX((gint)bg->blue - BACKDROP_VALUE, 0);
	color.green = MAX((gint)bg->green - BACKDROP_VALUE, 0);
	if (!color.red && !color.blue && !color.green) {
		color.red = MIN((gint)bg->red + BACKDROP_VALUE, G_MAXUINT16);
		color.blue = MIN((gint)bg->blue + BACKDROP_VALUE, G_MAXUINT16);
		color.green = MIN((gint)bg->green + BACKDROP_VALUE, G_MAXUINT16);
	}
	gtk_widget_modify_fg(widget, gtk_widget_get_state(widget), &color);

	gdk_draw_rectangle(GDK_DRAWABLE(trans->priv->layer_text),
			   widget->style->fg_gc[gtk_widget_get_state(widget)],
			   TRUE,
			   0, y_start,
			   widget->allocation.width, y_end - y_start);
}

/** @private */
G_GNUC_INTERNAL void
gtk_experiment_transcript_text_layer_redraw(GtkExperimentTranscript *trans)
{
	GtkWidget *widget = GTK_WIDGET(trans);

	gint64 current_time = 0, current_time_px;
	gint last_contrib_y = -1;

	GtkExperimentTranscriptContribRenderer renderer;

	if (trans->priv->time_adjustment != NULL) {
		GtkAdjustment *adj =
				GTK_ADJUSTMENT(trans->priv->time_adjustment);
		current_time = (gint64)gtk_adjustment_get_value(adj);
	}
	current_time_px = TIME_TO_PX(current_time);

	gdk_draw_rectangle(GDK_DRAWABLE(trans->priv->layer_text),
			   widget->style->bg_gc[gtk_widget_get_state(widget)],
			   TRUE,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);

	render_backdrop_area(trans, current_time_px);

	gtk_widget_queue_draw_area(widget, 0, 0,
				   widget->allocation.width,
				   widget->allocation.height);

	if (trans->priv->contribs == NULL)
		return;

	renderer = gtk_experiment_transcript_get_reverse_mode(trans)
			? render_contribution_topdown
			: render_contribution_bottomup;

	for (GList *cur = experiment_reader_get_contribution_by_time(
							trans->priv->contribs,
							current_time);
	     cur != NULL;
	     cur = cur->prev) {
		ExperimentReaderContrib *contrib = (ExperimentReaderContrib *)
						   cur->data;

		if (!renderer(trans, contrib, current_time, current_time_px,
			      &last_contrib_y))
			break;
	}
}

static void
state_changed(GtkWidget *widget, GtkStateType state __attribute__((unused)))
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(widget);

	if (gtk_widget_get_realized(widget) &&
	    trans->priv->layer_text != NULL)
		gtk_experiment_transcript_text_layer_redraw(trans);
}

static gboolean
button_pressed(GtkWidget *widget, GdkEventButton *event)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(widget);

	gtk_widget_grab_focus(widget);

	/* Ignore double-clicks and triple-clicks */
	if (event->button != 3 || event->type != GDK_BUTTON_PRESS)
		return FALSE;

	gtk_menu_popup(GTK_MENU(trans->priv->menu), NULL, NULL, NULL, NULL,
                       event->button, event->time);
	return TRUE;
}

static gboolean
scrolled(GtkWidget *widget, GdkEventScroll *event)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(widget);

	GtkAdjustment *adj;
	gdouble value, real_upper;

	if (trans->priv->time_adjustment == NULL)
		return FALSE;

	adj = GTK_ADJUSTMENT(trans->priv->time_adjustment);
	value = gtk_adjustment_get_value(adj);
	real_upper = gtk_adjustment_get_upper(adj) -
		     gtk_adjustment_get_page_size(adj);

	switch (event->direction) {
	case GDK_SCROLL_UP:
		value -= gtk_adjustment_get_step_increment(adj);
		break;
	case GDK_SCROLL_DOWN:
		value += gtk_adjustment_get_step_increment(adj);
	default:
		break;
	}

	gtk_adjustment_set_value(adj, MIN(value, real_upper));

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
choose_text_color_activated(GtkWidget *widget __attribute__((unused)),
			    gpointer data)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(data);

	GtkWidget *dialog, *colorsel;

	dialog = gtk_color_selection_dialog_new("Choose Text Color...");
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
text_alignment_activated(GtkWidget *widget __attribute__((unused)),
			 gpointer data)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(data);
	PangoAlignment alignment;

	if (trans->priv->layer_text_layout == NULL)
		return;

	alignment = PANGO_ALIGN_RIGHT;
	for (GSList *cur = trans->priv->alignment_group;
	     cur != NULL;
	     cur = cur->next, alignment--) {
		if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(cur->data)))
			continue;

		pango_layout_set_alignment(trans->priv->layer_text_layout,
					   alignment);

		if (gtk_widget_get_realized(GTK_WIDGET(trans)) &&
		    trans->priv->layer_text != NULL)
			gtk_experiment_transcript_text_layer_redraw(trans);

		break;
	}
}

static void
reverse_activated(GtkWidget *widget, gpointer data)
{
	GtkExperimentTranscript *trans = GTK_EXPERIMENT_TRANSCRIPT(data);

	trans->priv->flag_mask &= ~GTK_EXPERIMENT_TRANSCRIPT_REVERSE_MASK;
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		trans->priv->flag_mask |= GTK_EXPERIMENT_TRANSCRIPT_REVERSE_MASK;

	if (gtk_widget_get_realized(GTK_WIDGET(trans)) &&
	    trans->priv->layer_text != NULL)
		gtk_experiment_transcript_text_layer_redraw(trans);
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

/**
 * @brief Load contributions from an experiment transcript file.
 *
 * The transcript file is given as an \e ExperimentReader object instance.
 * Only contributions (every text fragment identified by a timepoint) of the
 * configured speaker are used.
 *
 * @param trans Widget instance
 * @param exp   \e ExperimentReader instance
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_experiment_transcript_load(GtkExperimentTranscript *trans,
			       ExperimentReader *exp)
{
	experiment_reader_free_contributions(trans->priv->contribs);
	trans->priv->contribs =
		experiment_reader_get_contributions_by_speaker(exp, trans->speaker);

	gtk_experiment_transcript_text_layer_redraw(trans);

	return trans->priv->contribs != NULL;
}

/**
 * @brief Load contributions from an experiment transcript file.
 *
 * The transcript file is given as a file name.
 * Only contributions (every text fragment identified by a timepoint) of the
 * configured speaker are used.
 *
 * @sa gtk_experiment_transcript_load
 *
 * @param trans    Widget instance
 * @param filename \e ExperimentReader instance
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_experiment_transcript_load_filename(GtkExperimentTranscript *trans,
					const gchar *filename)
{
	gboolean res = FALSE;
	ExperimentReader *exp = experiment_reader_new(filename);

	if (exp != NULL) {
		res = gtk_experiment_transcript_load(trans, exp);
		g_object_unref(exp);
	}

	return res;
}

/**
 * @brief Enable or disable drawing a backdrop area
 *
 * @param trans Widget instance
 * @param use   Whether to enable (\c TRUE) or disable (\c FALSE) the backdrop
 *              area
 */
void
gtk_experiment_transcript_set_use_backdrop_area(GtkExperimentTranscript *trans,
						gboolean use)
{
	trans->priv->flag_mask &= ~GTK_EXPERIMENT_TRANSCRIPT_USE_BACKDROP_MASK;
	trans->priv->flag_mask |=
			use ? GTK_EXPERIMENT_TRANSCRIPT_USE_BACKDROP_MASK : 0;

	if (gtk_widget_get_realized(GTK_WIDGET(trans)) &&
	    trans->priv->layer_text != NULL)
		gtk_experiment_transcript_text_layer_redraw(trans);
}

/**
 * @brief Retrieve whether a backdrop area is drawn or not
 *
 * @param trans Widget instance
 * @return \c TRUE if it is, else \c FALSE
 */
gboolean
gtk_experiment_transcript_get_use_backdrop_area(GtkExperimentTranscript *trans)
{
	return trans->priv->flag_mask &
	       GTK_EXPERIMENT_TRANSCRIPT_USE_BACKDROP_MASK;
}

/**
 * @brief Configure transcript's backdrop area
 *
 * Set a highlighted area of the transcript by specifying start
 * and end timepoints. The highlighted area is drawn with a 16% lighter or
 * darker background color than the widgets current background color.
 * It is ownly drawn when the backdrop area is enabled.
 *
 * @sa gtk_experiment_transcript_set_use_backdrop_area
 *
 * @param trans Widget instance
 * @param start Start time in milliseconds
 * @param end   End time in milliseconds
 */
void
gtk_experiment_transcript_set_backdrop_area(GtkExperimentTranscript *trans,
					    gint64 start, gint64 end)
{
	start = MAX(start, 0);
	end = MAX(end, 0);

	if (start <= end) {
		trans->priv->backdrop.start = start;
		trans->priv->backdrop.end = end;
	} else {
		trans->priv->backdrop.end = start;
		trans->priv->backdrop.start = end;
	}

	if (!gtk_experiment_transcript_get_use_backdrop_area(trans))
		return;

	if (gtk_widget_get_realized(GTK_WIDGET(trans)) &&
	    trans->priv->layer_text != NULL)
		gtk_experiment_transcript_text_layer_redraw(trans);
}

/**
 * @brief Set or unset reverse (top-down) render mode
 *
 * The render mode defaults to bottom-up mode for new widget instances.
 *
 * @param trans   Widget instance
 * @param reverse Activate reverse-mode (\c TRUE), or deactivate (\c FALSE)
 */
void
gtk_experiment_transcript_set_reverse_mode(GtkExperimentTranscript *trans,
					   gboolean reverse)
{
	GtkCheckMenuItem *item =
			GTK_CHECK_MENU_ITEM(trans->priv->menu_reverse_item);

	gtk_check_menu_item_set_active(item, reverse);
}

/**
 * @brief Get current reverse (top-down) render mode state
 *
 * @sa gtk_experiment_transcript_set_reverse_mode
 *
 * @param trans Widget instance
 * @return \c TRUE if reverse-mode is active, else \c FALSE
 */
gboolean
gtk_experiment_transcript_get_reverse_mode(GtkExperimentTranscript *trans)
{
	return trans->priv->flag_mask & GTK_EXPERIMENT_TRANSCRIPT_REVERSE_MASK;
}

/**
 * @brief Set alignment (justification) of text blocks in a transcript widget
 *
 * The alignment defaults to \c PANGO_ALIGN_LEFT for new widget instances.
 *
 * @param trans     Widget instance
 * @param alignment New text alignment
 */
void
gtk_experiment_transcript_set_alignment(GtkExperimentTranscript *trans,
					PangoAlignment alignment)
{
	PangoAlignment cur_alignment = PANGO_ALIGN_RIGHT;

	for (GSList *cur = trans->priv->alignment_group;
	     cur != NULL;
	     cur = cur->next, cur_alignment--)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(cur->data),
					       cur_alignment == alignment);
}

/**
 * @brief Get alignment (justification) of text blocks in a transcript widget
 *
 * @sa gtk_experiment_transcript_set_alignment
 *
 * @param trans Widget instance
 * @return Current text alignment
 */
PangoAlignment
gtk_experiment_transcript_get_alignment(GtkExperimentTranscript *trans)
{
	if (trans->priv->layer_text_layout == NULL)
		return PANGO_ALIGN_LEFT;

	return pango_layout_get_alignment(trans->priv->layer_text_layout);
}

/**
 * @brief Get time-adjustment currently used by a transcript widget
 *
 * The time-adjustment is the only way to control the portion of the transcript
 * displayed by the widget.
 * The adjustment's value is in milliseconds.
 * The widget will initialize one on construction, so there \e should always be
 * an adjustment to get.
 *
 * @param trans Widget instance
 * @return Currently used time-adjustment
 */

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
 * @param trans Widget instance
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
