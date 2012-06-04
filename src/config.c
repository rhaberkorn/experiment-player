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

void
config_generic_set_transcript_font(const gchar *actor, const gchar *key,
				   const PangoFontDescription *font)
{
	gchar	group[255];
	gchar	*font_name;

	g_snprintf(group, sizeof(group), "%s Transcript", actor);

	if (font == NULL) {
		g_key_file_remove_key(keyfile, group, key, NULL);
	} else {
		font_name = pango_font_description_to_string(font);
		g_key_file_set_string(keyfile, group, key, font_name);
		g_free(font_name);
	}
}

PangoFontDescription *
config_generic_get_transcript_font(const gchar *actor, const gchar *key)
{
	gchar			group[255];
	gchar			*font_name;
	PangoFontDescription	*font_desc = NULL;

	g_snprintf(group, sizeof(group), "%s Transcript", actor);
	font_name = g_key_file_get_string(keyfile, group, key, NULL);
	if (font_name != NULL) {
		font_desc = pango_font_description_from_string(font_name);
		g_free(font_name);
	}

	return font_desc;
}

void
config_generic_set_transcript_color(const gchar *actor, const gchar *key,
				    const GdkColor *color)
{
	gchar	group[255];
	gchar	*color_name;

	g_snprintf(group, sizeof(group), "%s Transcript", actor);

	if (color == NULL) {
		g_key_file_remove_key(keyfile, group, key, NULL);
	} else {
		color_name = gdk_color_to_string(color);
		g_key_file_set_string(keyfile, group, key, color_name);
		g_free(color_name);
	}
}

gboolean
config_generic_get_transcript_color(const gchar *actor, const gchar *key,
				    GdkColor *color)
{
	gchar	group[255];
	gchar	*color_name;

	g_snprintf(group, sizeof(group), "%s Transcript", actor);
	color_name = g_key_file_get_string(keyfile, group, key, NULL);
	if (color_name == NULL)
		return FALSE;

	gdk_color_parse(color_name, color);
	g_free(color_name);

	return TRUE;
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
