/**
 * @file
 * "Format" expression-related functions of the \e GtkExperimentTranscript
 * widget
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

static gboolean
gtk_experiment_transcript_parse_format(GtkExperimentTranscriptFormat *fmt,
				       const gchar *str)
{
	PangoAttrIterator *iter;

	gchar *pattern, pattern_captures[255], *p;

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
		}
	} while (pango_attr_iterator_next(iter));
	pango_attr_iterator_destroy(iter);
	*p = '\0';
	g_free(pattern);

	fmt->regexp = g_regex_new(pattern_captures, G_REGEX_CASELESS, 0, NULL);
	if (fmt->regexp == NULL) {
		pango_attr_list_unref(fmt->attribs);
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

	g_regex_match(fmt->regexp, text, 0, &match_info);

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

gboolean
gtk_experiment_transcript_load_formats(GtkExperimentTranscript *trans,
				       const gchar *filename)
{
	FILE *file;
	gchar buf[255];

	gtk_experiment_transcript_free_formats(trans->priv->formats);
	trans->priv->formats = NULL;

	if (filename == NULL || !*filename)
		goto redraw;

	if ((file = g_fopen(filename, "r")) == NULL)
		return TRUE;

	while (fgets((char *)buf, sizeof(buf)-1, file) != NULL) {
		GtkExperimentTranscriptFormat *fmt;

		g_strchug(buf);

		switch (*buf) {
		case '#':
		case '\n':
		case '\0':
			continue;
		}

		fmt = g_new(GtkExperimentTranscriptFormat, 1);

		if (gtk_experiment_transcript_parse_format(fmt, buf)) {
			g_free(fmt);
			fclose(file);
			return TRUE;
		}

		trans->priv->formats = g_slist_prepend(trans->priv->formats, fmt);
	}
	trans->priv->formats = g_slist_reverse(trans->priv->formats);

	fclose(file);

redraw:
	gtk_experiment_transcript_text_layer_redraw(trans);
	return FALSE;
}

gboolean
gtk_experiment_transcript_set_interactive_format(GtkExperimentTranscript *trans,
						 const gchar *format,
						 gboolean with_markup)
{
	gboolean res = TRUE;

	gtk_experiment_transcript_free_format(&trans->priv->interactive_format);
	trans->priv->interactive_format.regexp = NULL;
	trans->priv->interactive_format.attribs = NULL;

	if (format == NULL || *format == '\0') {
		res = FALSE;
	} else if (with_markup) {
		res = gtk_experiment_transcript_parse_format(&trans->priv->interactive_format,
							     format);
	} else {
		/** @todo !with_markup, default attributes */
		res = TRUE;
	}

	gtk_experiment_transcript_text_layer_redraw(trans);
	return res;
}
