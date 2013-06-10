/**
 * @file
 * "Format" expression-related functions of the \e GtkExperimentTranscript
 * widget
 */

/*
 * Copyright (C) 2012-2013 Otto-von-Guericke-Universität Magdeburg
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
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "gtk-experiment-transcript.h"
#include "gtk-experiment-transcript-private.h"

static inline gint attr_list_get_length(PangoAttrList *list);
static gboolean gtk_experiment_transcript_parse_format(GtkExperimentTranscriptFormat *fmt,
						       const gchar *str,
						       GError **error);

#define FORMAT_REGEX_COMPILE_FLAGS	(G_REGEX_CASELESS)
#define FORMAT_REGEX_MATCH_FLAGS	(0)

static inline gint
attr_list_get_length(PangoAttrList *list)
{
	PangoAttrIterator *iter = pango_attr_list_get_iterator(list);
	gint length = 0;

	do
		length++;
	while (pango_attr_iterator_next(iter));
	pango_attr_iterator_destroy(iter);

	return length;
}

static gboolean
gtk_experiment_transcript_parse_format(GtkExperimentTranscriptFormat *fmt,
				       const gchar *str,
				       GError **error)
{
	PangoAttrIterator *iter;

	gchar *pattern, *pattern_captures, *p;
	gint capture_count = 0;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	if (!pango_parse_markup(str, -1, 0, &fmt->attribs, &pattern,
				NULL, error))
		return FALSE;

	p = pattern_captures = g_malloc(strlen(pattern) + 1 +
					2*attr_list_get_length(fmt->attribs));
	iter = pango_attr_list_get_iterator(fmt->attribs);
	do {
		gint start, end;

		pango_attr_iterator_range(iter, &start, &end);
		if (end == G_MAXINT)
			end = strlen(pattern);

		if (end - start > 0) {
			*p++ = '(';
			strncpy(p, pattern + start, end - start);
			p += end - start;
			*p++ = ')';

			capture_count++;
		}
	} while (pango_attr_iterator_next(iter));
	pango_attr_iterator_destroy(iter);
	*p = '\0';
	g_free(pattern);

	fmt->regexp = g_regex_new(pattern_captures,
				  FORMAT_REGEX_COMPILE_FLAGS, 0, error);
	g_free(pattern_captures);
	if (fmt->regexp == NULL) {
		gtk_experiment_transcript_free_format(fmt);
		fmt->attribs = NULL;

		return FALSE;
	}

	if (g_regex_get_capture_count(fmt->regexp) != capture_count) {
		g_set_error(error,
			    GTK_EXPERIMENT_TRANSCRIPT_ERROR,
			    GTK_EXPERIMENT_TRANSCRIPT_ERROR_REGEXCAPTURES,
			    "Additional regular expression captures not allowed");

		gtk_experiment_transcript_free_format(fmt);
		fmt->regexp = NULL;
		fmt->attribs = NULL;

		return FALSE;
	}

	return TRUE;
}

/** @private */
G_GNUC_INTERNAL void
gtk_experiment_transcript_apply_format(GtkExperimentTranscriptFormat *fmt,
				       const gchar *text,
				       PangoAttrList *attrib_list)
{
	GMatchInfo *match_info;

	if (fmt->regexp == NULL || fmt->attribs == NULL)
		return;

	g_regex_match(fmt->regexp, text, FORMAT_REGEX_MATCH_FLAGS, &match_info);

	while (g_match_info_matches(match_info)) {
		PangoAttrIterator *iter;
		gint match_num = 0;

		iter = pango_attr_list_get_iterator(fmt->attribs);
		do {
			gint start, end;
			GSList *attribs;

			pango_attr_iterator_range(iter, &start, &end);
			if (end == G_MAXINT)
				end = strlen(g_regex_get_pattern(fmt->regexp));

			if (end - start == 0)
				continue;

			start = end = -1;
			g_match_info_fetch_pos(match_info, ++match_num,
					       &start, &end);
			if (start < 0 || end < 0)
				continue;

			attribs = pango_attr_iterator_get_attrs(iter);
			for (GSList *cur = attribs; cur != NULL; cur = cur->next) {
				PangoAttribute *attrib;

				attrib = pango_attribute_copy((PangoAttribute *)cur->data);
				attrib->start_index = (guint)start;
				attrib->end_index = (guint)end;
				pango_attr_list_change(attrib_list, attrib);
			}
			g_slist_free(attribs);
		} while (pango_attr_iterator_next(iter));
		pango_attr_iterator_destroy(iter);

		g_match_info_next(match_info, NULL);
	}

	g_match_info_free(match_info);
}

/** @private */
G_GNUC_INTERNAL void
gtk_experiment_transcript_free_formats(GSList *formats)
{
	for (GSList *cur = formats; cur != NULL; cur = cur->next) {
		GtkExperimentTranscriptFormat *fmt =
				(GtkExperimentTranscriptFormat *)cur->data;

		gtk_experiment_transcript_free_format(fmt);
		g_free(fmt);
	}

	g_slist_free(formats);
}

/*
 * API
 */

/**
 * @brief Load a format file to use with the transcript widget
 *
 * Loading a format file applies additional formattings (highlighting) to the
 * transcript's contributions according to the rules specified in the file.
 * For information about the format file syntax and semantics, refer to the
 * "Experiment Player" manual.
 *
 * The format file is parsed and and compiled to an internal representation.
 *
 * @param trans    Widget instance
 * @param filename File name of format file to load (\c NULL or empty string
 *                 resets any formattings of a previously loaded file)
 * @param error    GError to set on failure, or \c NULL
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_experiment_transcript_load_formats(GtkExperimentTranscript *trans,
				       const gchar *filename,
				       GError **error)
{
	FILE *file;
	gchar buf[1024];
	gint cur_line = 0;

	gboolean res = FALSE;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	gtk_experiment_transcript_free_formats(trans->priv->formats);
	trans->priv->formats = NULL;

	if (filename == NULL || !*filename) {
		res = TRUE;
		goto redraw;
	}

	if ((file = g_fopen(filename, "r")) == NULL) {
		g_set_error(error,
			    GTK_EXPERIMENT_TRANSCRIPT_ERROR,
			    GTK_EXPERIMENT_TRANSCRIPT_ERROR_FILEOPEN,
			    "Failed to open format file \"%s\":\n%s",
			    filename, g_strerror(errno));

		goto redraw;
	}

	while (fgets((char *)buf, sizeof(buf), file) != NULL) {
		GtkExperimentTranscriptFormat *fmt;

		cur_line++;

		if (!feof(file) && !is_newline(buf[strlen(buf) - 1])) {
			g_set_error(error,
				    GTK_EXPERIMENT_TRANSCRIPT_ERROR,
				    GTK_EXPERIMENT_TRANSCRIPT_ERROR_LINELENGTH,
				    "Line %d in file \"%s\" is too long. "
				    "It must be less than %d characters.",
				    cur_line, filename, (int)sizeof(buf));

			fclose(file);
			gtk_experiment_transcript_free_formats(trans->priv->formats);
			trans->priv->formats = NULL;

			goto redraw;
		}

		g_strchug(buf);

		/* strip new line chars from end of `buf' */
		for (gchar *p = buf + strlen(buf) - 1;
		     p >= buf && is_newline(*p);
		     p--)
			*p = '\0';

		if (*buf == '#' || *buf == '\0')
			continue;

		fmt = g_new(GtkExperimentTranscriptFormat, 1);

		if (!gtk_experiment_transcript_parse_format(fmt, buf, error)) {
			g_prefix_error(error,
				       "Error parsing \"%s\" on line %d:\n",
				       filename, cur_line);

			g_free(fmt);
			fclose(file);
			gtk_experiment_transcript_free_formats(trans->priv->formats);
			trans->priv->formats = NULL;

			goto redraw;
		}

		trans->priv->formats = g_slist_prepend(trans->priv->formats, fmt);
	}
	trans->priv->formats = g_slist_reverse(trans->priv->formats);

	fclose(file);
	res = TRUE;

redraw:
	gtk_experiment_transcript_text_layer_redraw(trans);
	return res;
}

/**
 * @brief Specify an interactive format string for a transcript widget
 *
 * Associates a single format rule with a transcript widget. The formatting
 * changes are cumulatively applied after any changes introduced by a format
 * file loaded into the widget. The interactive format rule is independant of
 * any format file rules, that is it is not reset when format file rules are
 * reset.
 * A rule may also be given without Pango markup (plain regular expression),
 * applying default formattings for that regular expression. Default formattings
 * may be configured via public instance attributes of the widget.
 * For information about the format rule syntax and semantics, refer to the
 * "Experiment Player" manual.
 *
 * @sa gtk_experiment_transcript_load_formats
 *
 * @param trans       Widget instance
 * @param format_str  Format rule string (with or without markup)
 * @param with_markup Must be \c TRUE if the format_str contains Pango markup,
 *                    else \c FALSE
 * @param error       GError to set on failure, or \c NULL
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_experiment_transcript_set_interactive_format(GtkExperimentTranscript *trans,
						 const gchar *format_str,
						 gboolean with_markup,
						 GError **error)
{
	GtkExperimentTranscriptFormat *fmt = &trans->priv->interactive_format;

	gchar *pattern;
	PangoAttribute *attrib;

	gboolean res = FALSE;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	gtk_experiment_transcript_free_format(fmt);
	fmt->regexp = NULL;
	fmt->attribs = NULL;

	if (format_str == NULL || !*format_str) {
		res = TRUE;
		goto redraw;
	}

	if (with_markup) {
		res = gtk_experiment_transcript_parse_format(fmt, format_str,
							     error);
		goto redraw;
	}
	/* else if (!with_markup) */

	fmt->attribs = pango_attr_list_new();
	g_warn_if_fail(fmt->attribs != NULL);
	if (fmt->attribs == NULL)
		goto redraw;

	if (trans->interactive_format.default_font != NULL) {
		attrib = pango_attr_font_desc_new(trans->interactive_format.default_font);
		attrib->end_index = 1;
		pango_attr_list_insert(fmt->attribs, attrib);
	}
	if (trans->interactive_format.default_text_color != NULL) {
		GdkColor *color = trans->interactive_format.default_text_color;

		attrib = pango_attr_foreground_new(color->red,
						   color->green,
						   color->blue);
		attrib->end_index = 1;
		pango_attr_list_insert(fmt->attribs, attrib);
	}
	if (trans->interactive_format.default_bg_color != NULL) {
		GdkColor *color = trans->interactive_format.default_bg_color;

		attrib = pango_attr_background_new(color->red,
						   color->green,
						   color->blue);
		attrib->end_index = 1;
		pango_attr_list_insert(fmt->attribs, attrib);
	}

	pattern = g_strconcat("(", format_str, ")", NULL);
	fmt->regexp = g_regex_new(pattern, FORMAT_REGEX_COMPILE_FLAGS, 0, error);
	g_free(pattern);
	if (fmt->regexp == NULL) {
		gtk_experiment_transcript_free_format(fmt);
		fmt->attribs = NULL;

		goto redraw;
	}

	if (g_regex_get_capture_count(fmt->regexp) != 1) {
		g_set_error(error,
			    GTK_EXPERIMENT_TRANSCRIPT_ERROR,
			    GTK_EXPERIMENT_TRANSCRIPT_ERROR_REGEXCAPTURES,
			    "Additional regular expression captures not allowed");

		gtk_experiment_transcript_free_format(fmt);
		fmt->regexp = NULL;
		fmt->attribs = NULL;

		goto redraw;
	}
	res = TRUE;

redraw:
	gtk_experiment_transcript_text_layer_redraw(trans);
	return res;
}
