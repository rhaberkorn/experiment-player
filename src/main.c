/**
 * @file
 * Program entry point, UI initialization, widget connections and GTK event
 * loop.
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

#include <locale.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#include <shellapi.h>
#endif

#include <glib.h>
#include <glib/gprintf.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>

#include <gtk-vlc-player.h>
#include <gtk-experiment-navigator.h>
#include <gtk-experiment-transcript.h>
#include <experiment-reader.h>

#include "experiment-player.h"

static inline void button_image_set_from_stock(GtkButton *widget,
					       const gchar *name);

GtkWidget *player_window,
	  *info_window,
	  *about_dialog;

GtkWidget *player_widget,
	  *controls_hbox,
	  *scale_widget,
	  *playpause_button,
	  *volume_button;

GtkWidget *player_window_statusbar;

GtkWidget *transcript_table,
	  *transcript_wizard_widget,
	  *transcript_proband_widget,
	  *transcript_scroll_widget;

GtkWidget *navigator_scrolledwindow,
	  *navigator_widget;

gchar *current_filename = NULL;

#define TOOLTIP_PLAY	"Start video playback"
#define TOOLTIP_PAUSE	"Pause video playback"

#define SPEAKER_WIZARD	"Wizard"
#define SPEAKER_PROBAND	"Proband"

/** @private */
GQuark
experiment_player_error_quark(void)
{
	return g_quark_from_static_string("experiment-player-error-quark");
}

/*
 * GtkBuilder signal callbacks
 * NOTE: for some strange reason the parameters are switched
 */

/** @private */
void
help_menu_about_item_activate_cb(GtkWidget *widget,
				 gpointer data __attribute__((unused)))
{
	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_hide(widget);
}

/** @private */
void
player_widget_length_changed_cb(GtkWidget *widget, gint64 new_length,
				gpointer data __attribute__((unused)))
{
	gtk_label_set_text(GTK_LABEL(widget),
			   format_timepoint("Length: ", new_length));
}

/** @private */
void
player_widget_time_changed_cb(GtkWidget *widget, gint64 new_time,
			      gpointer data __attribute__((unused)))
{
	gtk_label_set_text(GTK_LABEL(widget),
			   format_timepoint("Time: ", new_time));
}

/** @private */
void
playpause_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	gboolean is_playing = gtk_vlc_player_toggle(GTK_VLC_PLAYER(widget));

	button_image_set_from_stock(GTK_BUTTON(data),
				    is_playing ? GTK_STOCK_MEDIA_PLAY
					       : GTK_STOCK_MEDIA_PAUSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(data),
				    is_playing ? TOOLTIP_PLAY
					       : TOOLTIP_PAUSE);
}

/** @private */
void
stop_button_clicked_cb(GtkWidget *widget,
		       gpointer data __attribute__((unused)))
{
	gtk_vlc_player_stop(GTK_VLC_PLAYER(widget));
	button_image_set_from_stock(GTK_BUTTON(playpause_button),
				    GTK_STOCK_MEDIA_PLAY);
	gtk_widget_set_tooltip_text(playpause_button, TOOLTIP_PLAY);
}

/** @private */
void
file_menu_openmovie_item_activate_cb(GtkWidget *widget,
				     gpointer data __attribute__((unused)))
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Open Video...", GTK_WINDOW(widget),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					     NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *file;

		file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		if (!load_media_file(file)) {
			/* TODO */
		}
		refresh_quickopen_menu(GTK_MENU(quickopen_menu));

		g_free(file);
	}

	gtk_widget_destroy(dialog);
}

/** @private */
void
file_menu_opentranscript_item_activate_cb(GtkWidget *widget,
					  gpointer data __attribute__((unused)))
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Open Transcript...", GTK_WINDOW(widget),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					     NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *file;

		file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		if (!load_transcript_file(file)) {
			/* TODO */
		}
		refresh_quickopen_menu(GTK_MENU(quickopen_menu));

		g_free(file);
	}

	gtk_widget_destroy(dialog);
}

/** @private */
void
help_menu_manual_item_activate_cb(GtkWidget *widget __attribute__((unused)),
				  gpointer data __attribute__((unused)))
{
	GError *error = NULL;
	gboolean res;

#ifdef HAVE_WINDOWS_H
	res = (int)ShellExecute(NULL, "open", HELP_URI,
				NULL, NULL, SW_SHOWNORMAL) > 32;
	if (!res)
		error = g_error_new(EXPERIMENT_PLAYER_ERROR,
				    EXPERIMENT_PLAYER_ERROR_OPEN,
				    "Cannot open \"%s\"!", HELP_URI);
#else
	res = gtk_show_uri(NULL, HELP_URI, GDK_CURRENT_TIME, &error);
#endif

	if (!res) {
		show_message_dialog_gerror(error);
		g_error_free(error);
	}
}

/** @private */
gboolean
player_window_delete_event_cb(GtkWidget *widget __attribute__((unused)),
			      GdkEvent *event __attribute__((unused)),
			      gpointer user_data __attribute__((unused)))
{
	gtk_main_quit();

	/* do not destroy window (yet) */
	return TRUE;
}

/** @private */
void
navigator_widget_time_selected_cb(GtkWidget *widget, gint64 selected_time,
				  gpointer user_data __attribute__((unused)))
{
	gtk_vlc_player_seek(GTK_VLC_PLAYER(widget), selected_time);
}

/** @private */
void
navigator_widget_section_activated_cb(GtkWidget *widget __attribute__((unused)),
				      gint64 start, gint64 end,
				      gpointer user_data __attribute__((unused)))
{
	GtkExperimentTranscript *transcript_wizard =
			GTK_EXPERIMENT_TRANSCRIPT(transcript_wizard_widget);
	GtkExperimentTranscript *transcript_proband =
			GTK_EXPERIMENT_TRANSCRIPT(transcript_proband_widget);

	gtk_experiment_transcript_set_backdrop_area(transcript_wizard,
						    start, end);
	gtk_experiment_transcript_set_backdrop_area(transcript_proband,
						    start, end);
}

/** @private */
gboolean
navigator_widget_generic_focus_event_cb(GtkWidget *widget __attribute__((unused)),
				        GdkEventFocus *event,
				        gpointer user_data __attribute__((unused)))
{
	GtkExperimentTranscript *transcript_wizard =
			GTK_EXPERIMENT_TRANSCRIPT(transcript_wizard_widget);
	GtkExperimentTranscript *transcript_proband =
			GTK_EXPERIMENT_TRANSCRIPT(transcript_proband_widget);

	gtk_experiment_transcript_set_use_backdrop_area(transcript_wizard,
							event->in == TRUE);
	gtk_experiment_transcript_set_use_backdrop_area(transcript_proband,
							event->in == TRUE);

	return TRUE;
}

/** @private */
void
generic_quit_cb(GtkWidget *widget __attribute__((unused)),
		gpointer data __attribute__((unused)))
{
	gtk_main_quit();
}

static inline void
button_image_set_from_stock(GtkButton *widget, const gchar *name)
{
	GtkWidget *image = gtk_bin_get_child(GTK_BIN(widget));

	gtk_image_set_from_stock(GTK_IMAGE(image), name,
				 GTK_ICON_SIZE_SMALL_TOOLBAR);
}

gboolean
load_media_file(const gchar *file)
{
	if (!gtk_vlc_player_load_filename(GTK_VLC_PLAYER(player_widget), file))
		return FALSE;

	g_free(current_filename);
	current_filename = g_strdup(file);

	gtk_widget_set_sensitive(controls_hbox, TRUE);

	button_image_set_from_stock(GTK_BUTTON(playpause_button),
				    GTK_STOCK_MEDIA_PLAY);
	gtk_widget_set_tooltip_text(playpause_button, TOOLTIP_PLAY);

	return TRUE;
}

gboolean
load_transcript_file(const gchar *file)
{
	ExperimentReader *reader;
	gboolean res;

	reader = experiment_reader_new(file);
	if (reader == NULL)
		return FALSE;

	res = gtk_experiment_transcript_load(GTK_EXPERIMENT_TRANSCRIPT(transcript_wizard_widget),
					     reader);
	if (!res) {
		g_object_unref(G_OBJECT(reader));
		return FALSE;
	}

	res = gtk_experiment_transcript_load(GTK_EXPERIMENT_TRANSCRIPT(transcript_proband_widget),
					     reader);
	if (!res) {
		g_object_unref(G_OBJECT(reader));
		return FALSE;
	}

	res = gtk_experiment_navigator_load(GTK_EXPERIMENT_NAVIGATOR(navigator_widget),
					    reader);
	if (!res) {
		g_object_unref(G_OBJECT(reader));
		return FALSE;
	}

	g_object_unref(reader);

	gtk_widget_set_sensitive(transcript_table, TRUE);
	gtk_widget_set_sensitive(navigator_scrolledwindow, TRUE);

	return TRUE;
}

void
show_message_dialog_gerror(GError *err)
{
	GtkWidget *dialog;

	if (err == NULL)
		return;

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"%s", err->message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

const gchar *
window_get_geometry(GtkWindow *window)
{
	static gchar geometry[255];
	gint width, height, x, y;

	gtk_window_get_size(window, &width, &height);
	gtk_window_get_position(window, &x, &y);
	g_snprintf(geometry, sizeof(geometry), "%dx%d+%d+%d",
		   width, height, x, y);

	return geometry;
}

void
window_set_state(GtkWindow *window, GdkWindowState state)
{
	/* the GDK_WINDOW_STATE_WITHDRAWN flag is ignored */

	if (state & GDK_WINDOW_STATE_ICONIFIED)
		gtk_window_iconify(window);
	else
		gtk_window_deiconify(window);

	if (state & GDK_WINDOW_STATE_MAXIMIZED)
		gtk_window_maximize(window);
	else
		gtk_window_unmaximize(window);

	if (state & GDK_WINDOW_STATE_STICKY)
		gtk_window_stick(window);
	else
		gtk_window_unstick(window);

	if (state & GDK_WINDOW_STATE_FULLSCREEN)
		gtk_window_fullscreen(window);
	else
		gtk_window_unfullscreen(window);

	if (state & GDK_WINDOW_STATE_ICONIFIED)
		gtk_window_iconify(window);
	else
		gtk_window_deiconify(window);

	gtk_window_set_keep_above(window, state & GDK_WINDOW_STATE_ABOVE);
	gtk_window_set_keep_below(window, state & GDK_WINDOW_STATE_BELOW);
}

/** @private */
int
main(int argc, char *argv[])
{
	GtkBuilder *builder;
	GtkExperimentTranscript *transcript_wizard, *transcript_proband;

	GtkAdjustment *adj;
	PangoFontDescription *font_desc;
	gboolean reverse;
	PangoAlignment alignment;
	GdkColor color;
	GtkRcStyle *modified_style;

	/* FIXME: support internationalization instead of enforcing English */
#ifdef G_OS_WIN32
	g_setenv("LC_ALL", "English", TRUE);
#else
	g_setenv("LC_ALL", "en", TRUE);
#endif
	setlocale(LC_ALL, "");

	/* init threads */
	g_thread_init(NULL);
	gdk_threads_init();

	gtk_init(&argc, &argv);
	g_set_prgname(PACKAGE_NAME);

	config_init_key_file();

	builder = gtk_builder_new();

	gtk_builder_add_from_file(builder, DEFAULT_UI, NULL);
	gtk_builder_connect_signals(builder, NULL);

	BUILDER_INIT(builder, player_window);
	BUILDER_INIT(builder, info_window);
	BUILDER_INIT(builder, about_dialog);

	BUILDER_INIT(builder, player_widget);
	BUILDER_INIT(builder, controls_hbox);
	BUILDER_INIT(builder, scale_widget);
	BUILDER_INIT(builder, playpause_button);
	BUILDER_INIT(builder, volume_button);

	BUILDER_INIT(builder, player_window_statusbar);

	BUILDER_INIT(builder, quickopen_menu);
	BUILDER_INIT(builder, quickopen_menu_empty_item);

	BUILDER_INIT(builder, transcript_table);
	BUILDER_INIT(builder, transcript_wizard_widget);
	transcript_wizard = GTK_EXPERIMENT_TRANSCRIPT(transcript_wizard_widget);
	BUILDER_INIT(builder, transcript_proband_widget);
	transcript_proband = GTK_EXPERIMENT_TRANSCRIPT(transcript_proband_widget);
	BUILDER_INIT(builder, transcript_scroll_widget);

	BUILDER_INIT(builder, transcript_wizard_combo);
	BUILDER_INIT(builder, transcript_proband_combo);
	BUILDER_INIT(builder, transcript_wizard_entry_check);
	BUILDER_INIT(builder, transcript_proband_entry_check);

	BUILDER_INIT(builder, navigator_scrolledwindow);
	BUILDER_INIT(builder, navigator_widget);

	g_object_unref(G_OBJECT(builder));

	/* configure about dialog */
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog),
					  PACKAGE_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog),
				     PACKAGE_VERSION);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog),
				     PACKAGE_URL);

	/** @todo most of this could be done in Glade with proper catalog files */
	/* connect timeline, volume button and other widgets with player widget */
	adj = gtk_vlc_player_get_time_adjustment(GTK_VLC_PLAYER(player_widget));
	gtk_range_set_adjustment(GTK_RANGE(scale_widget), adj);
	gtk_experiment_transcript_set_time_adjustment(GTK_EXPERIMENT_TRANSCRIPT(transcript_wizard_widget),
						      adj);
	gtk_experiment_transcript_set_time_adjustment(GTK_EXPERIMENT_TRANSCRIPT(transcript_proband_widget),
						      adj);
	gtk_range_set_adjustment(GTK_RANGE(transcript_scroll_widget), adj);

	adj = gtk_vlc_player_get_volume_adjustment(GTK_VLC_PLAYER(player_widget));
	gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON(volume_button), adj);

	/*
	 * configure transcript widgets
	 */
	transcript_wizard->speaker = g_strdup(SPEAKER_WIZARD);
	font_desc = config_get_transcript_font(SPEAKER_WIZARD);
	if (font_desc != NULL) {
		gtk_widget_modify_font(transcript_wizard_widget, font_desc);
		pango_font_description_free(font_desc);
	}
	if (config_get_transcript_text_color(SPEAKER_WIZARD, &color))
		gtk_widget_modify_text(transcript_wizard_widget,
				       GTK_STATE_NORMAL, &color);
	if (config_get_transcript_bg_color(SPEAKER_WIZARD, &color))
		gtk_widget_modify_bg(transcript_wizard_widget,
				     GTK_STATE_NORMAL, &color);

	reverse = config_get_transcript_reverse_mode(SPEAKER_WIZARD);
	gtk_experiment_transcript_set_reverse_mode(transcript_wizard, reverse);

	alignment = config_get_transcript_alignment(SPEAKER_WIZARD);
	gtk_experiment_transcript_set_alignment(transcript_wizard, alignment);

	transcript_wizard->interactive_format.default_font =
			config_get_transcript_default_format_font(SPEAKER_WIZARD);
	if (config_get_transcript_default_format_text_color(SPEAKER_WIZARD, &color))
		transcript_wizard->interactive_format.default_text_color = gdk_color_copy(&color);
	if (config_get_transcript_default_format_bg_color(SPEAKER_WIZARD, &color))
		transcript_wizard->interactive_format.default_bg_color = gdk_color_copy(&color);

	transcript_proband->speaker = g_strdup(SPEAKER_PROBAND);
	font_desc = config_get_transcript_font(SPEAKER_PROBAND);
	if (font_desc != NULL) {
		gtk_widget_modify_font(transcript_proband_widget, font_desc);
		pango_font_description_free(font_desc);
	}
	if (config_get_transcript_text_color(SPEAKER_PROBAND, &color))
		gtk_widget_modify_text(transcript_proband_widget,
				       GTK_STATE_NORMAL, &color);
	if (config_get_transcript_bg_color(SPEAKER_PROBAND, &color))
		gtk_widget_modify_bg(transcript_proband_widget,
				     GTK_STATE_NORMAL, &color);

	reverse = config_get_transcript_reverse_mode(SPEAKER_PROBAND);
	gtk_experiment_transcript_set_reverse_mode(transcript_proband, reverse);

	alignment = config_get_transcript_alignment(SPEAKER_PROBAND);
	gtk_experiment_transcript_set_alignment(transcript_proband, alignment);

	transcript_proband->interactive_format.default_font =
			config_get_transcript_default_format_font(SPEAKER_PROBAND);
	if (config_get_transcript_default_format_text_color(SPEAKER_PROBAND, &color))
		transcript_proband->interactive_format.default_text_color = gdk_color_copy(&color);
	if (config_get_transcript_default_format_bg_color(SPEAKER_PROBAND, &color))
		transcript_proband->interactive_format.default_bg_color = gdk_color_copy(&color);

	format_selection_init();

	refresh_quickopen_menu(GTK_MENU(quickopen_menu));

	/* configure windows */
	gtk_window_set_gravity(GTK_WINDOW(player_window), GDK_GRAVITY_STATIC);
	gtk_window_set_gravity(GTK_WINDOW(info_window), GDK_GRAVITY_STATIC);

	if (config_get_save_window_properties()) {
		gchar *geometry;

		geometry = config_get_window_geometry("Player");
		if (geometry != NULL)
			gtk_window_parse_geometry(GTK_WINDOW(player_window),
						  geometry);
		g_free(geometry);

		window_set_state(GTK_WINDOW(player_window),
				 config_get_window_state("Player"));

		geometry = config_get_window_geometry("Info");
		if (geometry != NULL)
			gtk_window_parse_geometry(GTK_WINDOW(info_window),
						  geometry);
		g_free(geometry);

		window_set_state(GTK_WINDOW(info_window),
				 config_get_window_state("Info"));
	}

	gtk_widget_show(player_window);
	gtk_widget_show(info_window);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	/*
	 * update config file
	 */
	if (config_get_save_window_properties()) {
		const gchar	*geometry;
		GdkWindowState	state;

		geometry = window_get_geometry(GTK_WINDOW(player_window));
		config_set_window_geometry("Player", geometry);

		state = gdk_window_get_state(gtk_widget_get_window(player_window));
		config_set_window_state("Player", state);

		geometry = window_get_geometry(GTK_WINDOW(info_window));
		config_set_window_geometry("Info", geometry);

		state = gdk_window_get_state(gtk_widget_get_window(info_window));
		config_set_window_state("Info", state);
	}

	modified_style = gtk_widget_get_modifier_style(transcript_wizard_widget);
	config_set_transcript_font(SPEAKER_WIZARD, modified_style->font_desc);
	config_set_transcript_text_color(SPEAKER_WIZARD,
					 modified_style->color_flags[GTK_STATE_NORMAL] & GTK_RC_TEXT
						? &modified_style->text[GTK_STATE_NORMAL]
						: NULL);
	config_set_transcript_bg_color(SPEAKER_WIZARD,
				       modified_style->color_flags[GTK_STATE_NORMAL] & GTK_RC_BG
						? &modified_style->bg[GTK_STATE_NORMAL]
						: NULL);
	g_object_unref(modified_style);

	reverse = gtk_experiment_transcript_get_reverse_mode(transcript_wizard);
	config_set_transcript_reverse_mode(SPEAKER_WIZARD, reverse);

	alignment = gtk_experiment_transcript_get_alignment(transcript_wizard);
	config_set_transcript_alignment(SPEAKER_WIZARD, alignment);

	config_set_transcript_default_format_font(SPEAKER_WIZARD,
						  transcript_wizard->interactive_format.default_font);
	config_set_transcript_default_format_text_color(SPEAKER_WIZARD,
							transcript_wizard->interactive_format.default_text_color);
	config_set_transcript_default_format_bg_color(SPEAKER_WIZARD,
						      transcript_wizard->interactive_format.default_bg_color);

	modified_style = gtk_widget_get_modifier_style(transcript_proband_widget);
	config_set_transcript_font(SPEAKER_PROBAND, modified_style->font_desc);
	config_set_transcript_text_color(SPEAKER_PROBAND,
					 modified_style->color_flags[GTK_STATE_NORMAL] & GTK_RC_TEXT
						? &modified_style->text[GTK_STATE_NORMAL]
						: NULL);
	config_set_transcript_bg_color(SPEAKER_PROBAND,
				       modified_style->color_flags[GTK_STATE_NORMAL] & GTK_RC_BG
						? &modified_style->bg[GTK_STATE_NORMAL]
						: NULL);
	g_object_unref(modified_style);

	reverse = gtk_experiment_transcript_get_reverse_mode(transcript_proband);
	config_set_transcript_reverse_mode(SPEAKER_PROBAND, reverse);

	alignment = gtk_experiment_transcript_get_alignment(transcript_proband);
	config_set_transcript_alignment(SPEAKER_PROBAND, alignment);

	config_set_transcript_default_format_font(SPEAKER_PROBAND,
						  transcript_proband->interactive_format.default_font);
	config_set_transcript_default_format_text_color(SPEAKER_PROBAND,
							transcript_proband->interactive_format.default_text_color);
	config_set_transcript_default_format_bg_color(SPEAKER_PROBAND,
						      transcript_proband->interactive_format.default_bg_color);

	config_save_key_file();

	/* destroy all widgets (clean up) */
	gtk_widget_destroy(player_window);
	/**
	 * @bug some g_object_unref during transcript widget destruction
	 *      results in assertions
	 */
	gtk_widget_destroy(info_window);

	return 0;
}