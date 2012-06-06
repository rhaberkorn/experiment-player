#ifndef __EXPERIMENT_PLAYER_H
#define __EXPERIMENT_PLAYER_H

#include <glib.h>
#include <glib/gprintf.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>

/*
 * main.c
 */
gboolean load_media_file(const gchar *file);
gboolean load_transcript_file(const gchar *file);

void show_message_dialog_gerror(GError *err);

extern GtkWidget *player_widget,
		 *controls_hbox,
		 *scale_widget,
		 *playpause_button,
		 *volume_button;

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

void config_generic_set_transcript_font(const gchar *actor, const gchar *key,
					const PangoFontDescription *font);
PangoFontDescription *config_generic_get_transcript_font(const gchar *actor,
							 const gchar *key);

void config_generic_set_transcript_color(const gchar *actor, const gchar *key,
					 const GdkColor *color);
gboolean config_generic_get_transcript_color(const gchar *actor, const gchar *key,
					     GdkColor *color);

static inline void
config_set_transcript_font(const gchar *actor, const PangoFontDescription *font)
{
	config_generic_set_transcript_font(actor, "Widget-Font", font);
}
static inline PangoFontDescription *
config_get_transcript_font(const gchar *actor)
{
	return config_generic_get_transcript_font(actor, "Widget-Font");
}

static inline void
config_set_transcript_text_color(const gchar *actor, const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Widget-Text-Color", color);
}
static inline gboolean
config_get_transcript_text_color(const gchar *actor, GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Widget-Text-Color",
						   color);
}

static inline void
config_set_transcript_bg_color(const gchar *actor, const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Widget-BG-Color", color);
}
static inline gboolean
config_get_transcript_bg_color(const gchar *actor, GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Widget-BG-Color",
						   color);
}

static inline void
config_set_transcript_default_format_font(const gchar *actor,
					  const PangoFontDescription *font)
{
	config_generic_set_transcript_font(actor, "Default-Format-Font", font);
}
static inline PangoFontDescription *
config_get_transcript_default_format_font(const gchar *actor)
{
	return config_generic_get_transcript_font(actor, "Default-Format-Font");
}

static inline void
config_set_transcript_default_format_text_color(const gchar *actor,
						const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Default-Format-Text-Color",
					    color);
}
static inline gboolean
config_get_transcript_default_format_text_color(const gchar *actor,
						GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Default-Format-Text-Color",
						   color);
}

static inline void
config_set_transcript_default_format_bg_color(const gchar *actor,
					      const GdkColor *color)
{
	config_generic_set_transcript_color(actor, "Default-Format-BG-Color",
					    color);
}
static inline gboolean
config_get_transcript_default_format_bg_color(const gchar *actor,
					      GdkColor *color)
{
	return config_generic_get_transcript_color(actor, "Default-Format-BG-Color",
						   color);
}

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
#define BUILDER_INIT(BUILDER, VAR) do {					\
	VAR = GTK_WIDGET(gtk_builder_get_object(BUILDER, #VAR));	\
} while (0)

static inline gchar *
path_strip_extension(const gchar *filename)
{
	gchar *ret = g_strdup(filename);
	gchar *p;

	if ((p = g_strrstr(ret, ".")) != NULL)
		*p = '\0';

	return ret;
}

#endif