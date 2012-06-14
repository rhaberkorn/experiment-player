/**
 * @file
 * Main program header
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

#ifndef __EXPERIMENT_PLAYER_H
#define __EXPERIMENT_PLAYER_H

#include <glib.h>
#include <glib/gprintf.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>

/** Main program error domain */
#define EXPERIMENT_PLAYER_ERROR \
	(experiment_player_error_quark())

/** Main program error codes */
typedef enum {
	EXPERIMENT_PLAYER_ERROR_OPEN	/**< Error opening file/URI */
} ExperimentPlayerError;

/*
 * main.c
 */
/** @private */
GQuark experiment_player_error_quark(void);

gboolean load_media_file(const gchar *file);
gboolean load_transcript_file(const gchar *file);

void show_message_dialog_gerror(GError *err);

extern GtkWidget *about_dialog;

extern GtkWidget *player_widget,
		 *controls_hbox,
		 *scale_widget,
		 *playpause_button,
		 *volume_button;

extern GtkWidget *player_window_statusbar;

extern GtkWidget *transcript_table,
		 *transcript_wizard_widget,
		 *transcript_proband_widget,
		 *transcript_scroll_widget;

extern GtkWidget *navigator_scrolledwindow,
		 *navigator_widget;

extern gchar *current_filename;

/*
 * config.c
 */
void config_init_key_file(void);

void config_set_quickopen_directory(const gchar *dir);
gchar *config_get_quickopen_directory(void);
void config_set_formats_directory(const gchar *dir);
gchar *config_get_formats_directory(void);

/** @private */
void config_generic_set_transcript_font(const gchar *actor, const gchar *key,
					const PangoFontDescription *font);
/** @private */
PangoFontDescription *config_generic_get_transcript_font(const gchar *actor,
							 const gchar *key);

/** @private */
void config_generic_set_transcript_color(const gchar *actor, const gchar *key,
					 const GdkColor *color);
/** @private */
gboolean config_generic_get_transcript_color(const gchar *actor, const gchar *key,
					     GdkColor *color);

/** @public */
static inline void
config_set_transcript_font(const gchar *actor, const PangoFontDescription *font)
{
	config_generic_set_transcript_font(actor, "Widget-Font", font);
}
/** @public */
static inline PangoFontDescription *
config_get_transcript_font(const gchar *actor)
{
	return config_generic_get_transcript_font(actor, "Widget-Font");
}

/** @public */
static inline void
config_set_transcript_text_color(const gchar *actor, const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Widget-Text-Color", color);
}
/** @public */
static inline gboolean
config_get_transcript_text_color(const gchar *actor, GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Widget-Text-Color",
						   color);
}

/** @public */
static inline void
config_set_transcript_bg_color(const gchar *actor, const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Widget-BG-Color", color);
}
/** @public */
static inline gboolean
config_get_transcript_bg_color(const gchar *actor, GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Widget-BG-Color",
						   color);
}

/** @public */
static inline void
config_set_transcript_default_format_font(const gchar *actor,
					  const PangoFontDescription *font)
{
	config_generic_set_transcript_font(actor, "Default-Format-Font", font);
}
/** @public */
static inline PangoFontDescription *
config_get_transcript_default_format_font(const gchar *actor)
{
	return config_generic_get_transcript_font(actor, "Default-Format-Font");
}

/** @public */
static inline void
config_set_transcript_default_format_text_color(const gchar *actor,
						const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Default-Format-Text-Color",
					    color);
}
/** @public */
static inline gboolean
config_get_transcript_default_format_text_color(const gchar *actor,
						GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Default-Format-Text-Color",
						   color);
}

/** @public */
static inline void
config_set_transcript_default_format_bg_color(const gchar *actor,
					      const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Default-Format-BG-Color",
					    color);
}
/** @public */
static inline gboolean
config_get_transcript_default_format_bg_color(const gchar *actor,
					      GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Default-Format-BG-Color",
						   color);
}

void config_set_transcript_reverse_mode(const gchar *actor, gboolean reverse);
gboolean config_get_transcript_reverse_mode(const gchar *actor);

void config_set_transcript_alignment(const gchar *actor,
				     PangoAlignment alignment);
PangoAlignment config_get_transcript_alignment(const gchar *actor);

void config_save_key_file(void);

/*
 * quick-open.c
 */
void refresh_quickopen_menu(GtkMenu *menu);

extern GtkWidget *quickopen_menu,
		 *quickopen_menu_empty_item;

/*
 * format-selection.c
 */
void format_selection_init(void);

extern GtkWidget *transcript_wizard_combo,
		 *transcript_proband_combo,
		 *transcript_wizard_entry_check,
		 *transcript_proband_entry_check;

/*
 * macros and inline functions
 */
/** @private */
#define BUILDER_INIT(BUILDER, VAR) do {					\
	VAR = GTK_WIDGET(gtk_builder_get_object(BUILDER, #VAR));	\
} while (0)

/** @public */
static inline gchar *
path_strip_extension(const gchar *filename)
{
	gchar *ret = g_strdup(filename);
	gchar *p;

	if ((p = g_strrstr(ret, ".")) != NULL)
		*p = '\0';

	return ret;
}

/** @public */
static inline const gchar *
format_timepoint(const gchar *prefix, gint64 timept)
{
	static gchar buf[255];

	g_snprintf(buf, sizeof(buf),
		   "%s%" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT,
		   prefix != NULL ? prefix : "",
		   timept / (1000*60), (timept/1000) % 60);

	return buf;
}

#endif