/**
 * @file
 * Configuration file handling functions
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

#include <stdio.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <gdk/gdk.h>

#include "experiment-player.h"

static inline void set_default_string(const gchar *group_name, const gchar *key,
				      const gchar *string);
static const gchar *get_group_by_actor(const gchar *actor);

static GKeyFile	*keyfile;
static gchar	*filename = NULL;

void
config_init_key_file(void)
{
	keyfile = g_key_file_new();

	/* may fail if no serialized configuration exists */
	g_key_file_load_from_data_dirs(keyfile, CONFIG_KEY_FILE, &filename,
				       G_KEY_FILE_KEEP_COMMENTS, NULL);

	if (filename == NULL)
		filename = g_build_filename(g_get_user_data_dir(),
					    CONFIG_KEY_FILE, NULL);

	/* initialize defaults */
	set_default_string("Directories", "Quick-Open", DEFAULT_QUICKOPEN_DIR);
	set_default_string("Directories", "Formats", DEFAULT_FORMATS_DIR);

#ifdef DEFAULT_INTERACTIVE_FORMAT_FONT
	set_default_string("Wizard Transcript", "Default-Format-Font",
			   DEFAULT_INTERACTIVE_FORMAT_FONT);
	set_default_string("Proband Transcript", "Default-Format-Font",
			   DEFAULT_INTERACTIVE_FORMAT_FONT);
#endif
#ifdef DEFAULT_INTERACTIVE_FORMAT_FGCOLOR
	set_default_string("Wizard Transcript", "Default-Format-Text-Color",
			   DEFAULT_INTERACTIVE_FORMAT_FGCOLOR);
	set_default_string("Proband Transcript", "Default-Format-Text-Color",
			   DEFAULT_INTERACTIVE_FORMAT_FGCOLOR);
#endif
#ifdef DEFAULT_INTERACTIVE_FORMAT_BGCOLOR
	set_default_string("Wizard Transcript", "Default-Format-BG-Color",
			   DEFAULT_INTERACTIVE_FORMAT_BGCOLOR);
	set_default_string("Proband Transcript", "Default-Format-BG-Color",
			   DEFAULT_INTERACTIVE_FORMAT_BGCOLOR);
#endif
}

static inline void
set_default_string(const gchar *group_name, const gchar *key,
		   const gchar *string)
{
	if (!g_key_file_has_key(keyfile, group_name, key, NULL))
		g_key_file_set_string(keyfile, group_name, key, string);
}

void
config_set_quickopen_directory(const gchar *dir)
{
	g_key_file_set_string(keyfile, "Directories", "Quick-Open", dir);
}

gchar *
config_get_quickopen_directory(void)
{
	return g_key_file_get_string(keyfile, "Directories", "Quick-Open", NULL);
}

void
config_set_formats_directory(const gchar *dir)
{
	g_key_file_set_string(keyfile, "Directories", "Formats", dir);
}

gchar *
config_get_formats_directory(void)
{
	return g_key_file_get_string(keyfile, "Directories", "Formats", NULL);
}

static const gchar *
get_group_by_actor(const gchar *actor)
{
	static gchar group[255];

	g_snprintf(group, sizeof(group), "%s Transcript", actor);
	return group;
}

/** @private */
void
config_generic_set_transcript_font(const gchar *actor, const gchar *key,
				   const PangoFontDescription *font)
{
	const gchar *group = get_group_by_actor(actor);

	if (font == NULL) {
		g_key_file_remove_key(keyfile, group, key, NULL);
	} else {
		gchar *font_name = pango_font_description_to_string(font);
		g_key_file_set_string(keyfile, group, key, font_name);
		g_free(font_name);
	}
}

/** @private */
PangoFontDescription *
config_generic_get_transcript_font(const gchar *actor, const gchar *key)
{
	gchar			*font_name;
	PangoFontDescription	*font_desc = NULL;

	font_name = g_key_file_get_string(keyfile, get_group_by_actor(actor),
					  key, NULL);
	if (font_name != NULL) {
		font_desc = pango_font_description_from_string(font_name);
		g_free(font_name);
	}

	return font_desc;
}

/** @private */
void
config_generic_set_transcript_color(const gchar *actor, const gchar *key,
				    const GdkColor *color)
{
	const gchar *group = get_group_by_actor(actor);

	if (color == NULL) {
		g_key_file_remove_key(keyfile, group, key, NULL);
	} else {
		gchar *color_name = gdk_color_to_string(color);
		g_key_file_set_string(keyfile, group, key, color_name);
		g_free(color_name);
	}
}

/** @private */
gboolean
config_generic_get_transcript_color(const gchar *actor, const gchar *key,
				    GdkColor *color)
{
	gchar *color_name;

	color_name = g_key_file_get_string(keyfile, get_group_by_actor(actor),
					   key, NULL);
	if (color_name == NULL)
		return FALSE;

	gdk_color_parse(color_name, color);
	g_free(color_name);

	return TRUE;
}

void
config_set_transcript_reverse_mode(const gchar *actor, gboolean reverse)
{
	g_key_file_set_boolean(keyfile, get_group_by_actor(actor),
			       "Widget-Reverse-Mode", reverse);
}

gboolean
config_get_transcript_reverse_mode(const gchar *actor)
{
	return g_key_file_get_boolean(keyfile, get_group_by_actor(actor),
				      "Widget-Reverse-Mode", NULL);
}

void
config_set_transcript_alignment(const gchar *actor, PangoAlignment alignment)
{
	static gchar **values = NULL;

	if (values == NULL) {
		gchar *possible_values;

		pango_parse_enum(PANGO_TYPE_ALIGNMENT, "", NULL,
				 FALSE, &possible_values);
		values = g_strsplit(possible_values, "/", 0);
		g_free(possible_values);
	}

	g_key_file_set_string(keyfile, get_group_by_actor(actor),
			      "Widget-Alignment", values[alignment]);
}

PangoAlignment
config_get_transcript_alignment(const gchar *actor)
{
	gchar	*value;
	gint	alignment = (gint)PANGO_ALIGN_LEFT;

	value = g_key_file_get_string(keyfile, get_group_by_actor(actor),
				      "Widget-Alignment", NULL);
	if (value == NULL)
		return PANGO_ALIGN_LEFT;

	pango_parse_enum(PANGO_TYPE_ALIGNMENT, value, &alignment, FALSE, NULL);
	g_free(value);

	return (PangoAlignment)alignment;
}

void
config_save_key_file(void)
{
	gchar	*data;
	gsize	length;
	FILE	*file;

	data = g_key_file_to_data(keyfile, &length, NULL);

	file = g_fopen(filename, "w");
	if (file != NULL) {
		fwrite(data, (size_t)length, 1, file);
		fclose(file);
	}

	g_free(data);
}
