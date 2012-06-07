/**
 * @file
 * "Format" expression-related functions of the \e GtkExperimentTranscript
 * widget
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
#include <string.h>
#include <assert.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "gtk-experiment-transcript-private.h"

#define FORMAT_REGEX_COMPILE_FLAGS	(G_REGEX_CASELESS)
#define FORMAT_REGEX_MATCH_FLAGS	(0)

static gboolean
gtk_experiment_transcript_parse_format(GtkExperimentTranscriptFormat *fmt,
				       const gchar *str)
{
	PangoAttrIterator *iter;

	gchar *pattern, pattern_captures[255], *p;
	gint capture_count = 0;

	if (!pango_parse_markup(str, -1, 0, &fmt->attribs, &pattern,
				NULL, NULL))
		return TRUE;

	p = pattern_captures;
	iter = pango_attr_list_get_iterator(fmt->attribs);
	do {
		gint start, end;

		pango_attr_iterator_range(iter, &start, &end);

		if (end < G_MAXINT) {
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
				  FORMAT_REGEX_COMPILE_FLAGS, 0, NULL);
	if (fmt->regexp == NULL ||
	    g_regex_get_capture_count(fmt->regexp) != capture_count) {
		gtk_experiment_transcript_free_format(fmt);
		fmt->regexp = NULL;
		fmt->attribs = NULL;

		return TRUE;
	}

	return FALSE;
}

/** @private */
void
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
		gint match_num = 1;

		iter = pango_attr_list_get_iterator(fmt->attribs);
		do {
			gint start, end;

			pango_attr_iterator_range(iter, &start, &end);
			if (end == G_MAXINT)
				continue;

			start = end = -1;
			g_match_info_fetch_pos(match_info, match_num,
					       &start, &end);
			if (start >= 0 && end >= 0) {
				GSList *attribs;

				attribs = pango_attr_iterator_get_attrs(iter);
				for (GSList *cur = attribs; cur != NULL; cur = cur->next) {
					PangoAttribute *attrib;

					attrib = pango_attribute_copy((PangoAttribute *)cur->data);
					attrib->start_index = (guint)start;
					attrib->end_index = (guint)end;
					pango_attr_list_change(attrib_list, attrib);
				}
				g_slist_free(attribs);
			}

			match_num++;
		} while (pango_attr_iterator_next(iter));
		pango_attr_iterator_destroy(iter);

		g_match_info_next(match_info, NULL);
	}

	g_match_info_free(match_info);
}

/** @private */
void
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

/** @todo report errors using GError */
gboolean
gtk_experiment_transcript_load_formats(GtkExperimentTranscript *trans,
				       const gchar *filename)
{
	FILE *file;
	gchar buf[255];
	gboolean res = TRUE;

	gtk_experiment_transcript_free_formats(trans->priv->formats);
	trans->priv->formats = NULL;

	if (filename == NULL || !*filename) {
		res = FALSE;
		goto redraw;
	}

	if ((file = g_fopen(filename, "r")) == NULL)
		goto redraw;

	while (fgets((char *)buf, sizeof(buf)-1, file) != NULL) {
		GtkExperimentTranscriptFormat *fmt;

		g_strchug(buf);

		switch (*buf) {
		case '#':
		case '\r':
		case '\n':
		case '\0':
			continue;
		}

		fmt = g_new(GtkExperimentTranscriptFormat, 1);

		if (gtk_experiment_transcript_parse_format(fmt, buf)) {
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
	res = FALSE;

redraw:
	gtk_experiment_transcript_text_layer_redraw(trans);
	return res;
}

gboolean
gtk_experiment_transcript_set_interactive_format(GtkExperimentTranscript *trans,
						 const gchar *format_str,
						 gboolean with_markup)
{
	GtkExperimentTranscriptFormat *fmt = &trans->priv->interactive_format;

	gchar *pattern;
	PangoAttribute *attrib;

	gboolean res = TRUE;

	gtk_experiment_transcript_free_format(fmt);
	fmt->regexp = NULL;
	fmt->attribs = NULL;

	if (format_str == NULL || !*format_str) {
		res = FALSE;
		goto redraw;
	}

	if (with_markup) {
		res = gtk_experiment_transcript_parse_format(fmt, format_str);
		goto redraw;
	}
	/* else if (!with_markup) */

	fmt->attribs = pango_attr_list_new();
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
	fmt->regexp = g_regex_new(pattern, FORMAT_REGEX_COMPILE_FLAGS, 0, NULL);
	g_free(pattern);
	if (fmt->regexp == NULL ||
	    g_regex_get_capture_count(fmt->regexp) != 1) {
		gtk_experiment_transcript_free_format(fmt);
		fmt->regexp = NULL;
		fmt->attribs = NULL;

		goto redraw;
	}
	res = FALSE;

redraw:
	gtk_experiment_transcript_text_layer_redraw(trans);
	return res;
}
