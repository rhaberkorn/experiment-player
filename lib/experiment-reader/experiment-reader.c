/**
 * @file
 * Auxiliary class to handle "session" XML files (augmented Folker).
 * It is a GObject that must be freed using \e g_object_unref.
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

#include <string.h>
#include <assert.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "experiment-reader.h"

static void experiment_reader_class_init(ExperimentReaderClass *klass);
static void experiment_reader_init(ExperimentReader *klass);
static void experiment_reader_finalize(GObject *gobject);

static gint64 get_timepoint_by_ref(xmlDoc *doc, xmlChar *ref);
static xmlNode *get_first_element(xmlNode *children, const gchar *name);
static gboolean generic_foreach_topic(ExperimentReader *reader, xmlNodeSet *nodes,
				      ExperimentReaderTopicCallback callback,
				      gpointer data);

static gint experiment_reader_contrib_cmp(const ExperimentReaderContrib *a,
					  const ExperimentReaderContrib *b);
static void insert_contribution(gint64 start_time, gchar *text, GList **list);
static inline void process_contribution(xmlDoc *doc, xmlNode *contrib,
					GList **list);

/** @private */
#define XML_CHAR(STR) \
	((const xmlChar *)(STR))

/** @private */
#define EXPERIMENT_READER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), EXPERIMENT_TYPE_READER, ExperimentReaderPrivate))

/** @private */
struct _ExperimentReaderPrivate {
	xmlDoc *doc;
};

/**
 * @private
 * Will create \e experiment_reader_get_type and set
 * \e experiment_reader_parent_class
 */
G_DEFINE_TYPE(ExperimentReader, experiment_reader, G_TYPE_OBJECT);

static void
experiment_reader_class_init(ExperimentReaderClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	/* gobject_class->dispose = experiment_reader_dispose; */
	gobject_class->finalize = experiment_reader_finalize;

	g_type_class_add_private(klass, sizeof(ExperimentReaderPrivate));
}

static void
experiment_reader_init(ExperimentReader *klass)
{
	klass->priv = EXPERIMENT_READER_GET_PRIVATE(klass);

	klass->priv->doc = NULL;
}

static void
experiment_reader_finalize(GObject *gobject)
{
	ExperimentReader *reader = EXPERIMENT_READER(gobject);

	if (reader->priv->doc != NULL)
		xmlFreeDoc(reader->priv->doc);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(experiment_reader_parent_class)->finalize(gobject);
}

static gint64
get_timepoint_by_ref(xmlDoc *doc, xmlChar *ref)
{
	xmlChar expr[255];

	xmlXPathContext	*xpathCtx;
	xmlXPathObject	*xpathObj;

	double value;

	xpathCtx = xmlXPathNewContext(doc);
	assert(xpathCtx != NULL);

	/** @todo precompile XPath expression */
	xmlStrPrintf(expr, sizeof(expr),
		     XML_CHAR("/session/timeline/"
			      "timepoint[@timepoint-id = '%s']/"
			      "@absolute-time"), ref);

	xpathObj = xmlXPathEvalExpression(expr, xpathCtx);
	assert(xpathObj != NULL);

	value = xmlXPathCastToNumber(xpathObj);

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);

	return (gint64)(value*1000.);
}

static xmlNode *
get_first_element(xmlNode *children, const gchar *name)
{
	for (xmlNode *cur = children; cur != NULL; cur = cur->next)
		if (cur->type == XML_ELEMENT_NODE &&
		    !g_strcmp0((const gchar *)cur->name, name))
			return cur;

	return NULL;
}

static gboolean
generic_foreach_topic(ExperimentReader *reader, xmlNodeSet *nodes,
		      ExperimentReaderTopicCallback callback, gpointer data)
{
	if (nodes == NULL)
		return TRUE;

	for (int i = 0; i < nodes->nodeNr; i++) {
		xmlNode *cur = nodes->nodeTab[i];
		xmlNode *contrib = get_first_element(cur->children,
						     "contribution");
		assert(cur != NULL && cur->type == XML_ELEMENT_NODE);

		xmlChar	*topic_id = xmlGetProp(cur, XML_CHAR("id"));
		gint64	start_time = -1;

		if (contrib != NULL) {
			xmlChar *contrib_start_ref;

			contrib_start_ref = xmlGetProp(contrib,
						       XML_CHAR("start-reference"));
			start_time = get_timepoint_by_ref(reader->priv->doc,
							  contrib_start_ref);
			xmlFree(contrib_start_ref);
		}

		callback(reader, (const gchar *)topic_id, start_time, data);

		xmlFree(topic_id);
	}

	return FALSE;
}

static gint
experiment_reader_contrib_cmp(const ExperimentReaderContrib *a,
			      const ExperimentReaderContrib *b)
{
	if (a->start_time < b->start_time)
		return -1;
	if (a->start_time > b->start_time)
		return 1;
	return 0;
}

static void
insert_contribution(gint64 start_time, gchar *text, GList **list)
{
	ExperimentReaderContrib *contrib;

	if (text == NULL)
		return;

	contrib = g_malloc(sizeof(ExperimentReaderContrib) + strlen(text) + 1);
	contrib->start_time = start_time;
	g_stpcpy(contrib->text, g_strchomp(text));

	*list = g_list_insert_sorted(*list, contrib,
				     (GCompareFunc)experiment_reader_contrib_cmp);
}

static inline void
process_contribution(xmlDoc *doc, xmlNode *contrib, GList **list)
{
	xmlChar *ref;
	gint64 start_time;

	gchar *text = NULL;

	ref = xmlGetProp(contrib, XML_CHAR("start-reference"));
	start_time = get_timepoint_by_ref(doc, ref);
	xmlFree(ref);

	for (xmlNode *cur = contrib->children; cur != NULL; cur = cur->next) {
		xmlChar *content;
		gchar *new;

		switch (cur->type) {
		case XML_TEXT_NODE:
			content = xmlNodeGetContent(cur);

			new = g_strconcat(text != NULL ? text : "",
					  g_strstrip((gchar *)content),
					  " ", NULL);
			g_free(text);
			text = new;

			xmlFree(content);
			break;

		case XML_ELEMENT_NODE:
			if (!xmlStrcmp(cur->name, XML_CHAR("pause"))) {
				xmlChar *duration;

				duration = xmlGetProp(cur, XML_CHAR("duration"));
				if (duration == NULL)
					break;

				if (!xmlStrcmp(duration, XML_CHAR("micro")) ||
				    !xmlStrcmp(duration, XML_CHAR("short")))
					new = g_strconcat(text != NULL ? text : "",
							  "... ", NULL);
				else if (text == NULL)
					new = g_strdup("...\n");
				else
					new = g_strconcat(g_strchomp(text),
							  "\n", NULL);
				g_free(text);
				text = new;

				xmlFree(duration);
			} else if (!xmlStrcmp(cur->name, XML_CHAR("time"))) {
				insert_contribution(start_time, text, list);
				g_free(text);
				text = NULL;

				ref = xmlGetProp(cur,
						 XML_CHAR("timepoint-reference"));
				start_time = get_timepoint_by_ref(doc, ref);
				xmlFree(ref);
			}
			break;

		default:
			break;
		}
	}

	insert_contribution(start_time, text, list);
	g_free(text);
}

/*
 * API
 */

/**
 * @brief Constructs a new ExperimentReader object
 *
 * @param filename Filename of XML file to open
 * @return A new \e ExperimentReader object. Free with \e g_object_unref.
 */
ExperimentReader *
experiment_reader_new(const gchar *filename)
{
	ExperimentReader *reader;

	reader = EXPERIMENT_READER(g_object_new(EXPERIMENT_TYPE_READER, NULL));
	reader->priv->doc = xmlParseFile(filename);
	if (reader->priv->doc == NULL) {
		g_object_unref(G_OBJECT(reader));
		return NULL;
	}

	/** @todo validate against session.dtd */

	return reader;
}

/**
 * @brief Retrieve list of contributions by speaker
 *
 * Returns a newly-allocated doubly-linked list of
 * \ref ExperimentReaderContrib structures representing all contributions
 * by a given speaker. Every text fragment with a \e timepoint reference is
 * considered a contribution.
 * The list is sorted by the contributions' start times, in ascending order.
 *
 * @sa ExperimentReaderContrib
 * @sa experiment_reader_get_contribution_by_time
 * @sa experiment_reader_free_contributions
 *
 * @param reader  \e ExperimentReader instance
 * @param speaker Full name of the speaker (e.g. "Wizard")
 * @return Newly allocated list of contributions (must be freed with
 *         \ref experiment_reader_free_contributions)
 */
GList *
experiment_reader_get_contributions_by_speaker(ExperimentReader *reader,
					       const gchar *speaker)
{
	GList		*list = NULL;

	xmlXPathContext	*xpathCtx;
	xmlXPathObject	*xpathObj;

	xmlChar expr[255];

	xpathCtx = xmlXPathNewContext(reader->priv->doc);

	/* Evaluate xpath expression */
	xmlStrPrintf(expr, sizeof(expr),
		     XML_CHAR("//contribution[@speaker-reference = "
			      "/session/speakers/speaker[name = '%s']/@speaker-id]"),
		     speaker);
	xpathObj = xmlXPathEvalExpression(expr, xpathCtx);

	for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++) {
		xmlNode *contrib = xpathObj->nodesetval->nodeTab[i];

		process_contribution(reader->priv->doc, contrib, &list);
	}

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);

	return list;
}

/**
 * @brief Get a contribution by time
 *
 * Gets the closest contribution after the specified time or the last one
 * if there is no contribution after the specified time.
 * The contribution is returned as a pointer into the contribution list
 * so that the list may be traversed by the caller.
 *
 * @param contribs List of \ref ExperimentReaderContrib structures as returned
 *                 by \ref experiment_reader_get_contributions_by_speaker
 * @param timept   Time in milliseconds
 * @return List of contributions beginning with the desired contribution.
 *         It is a pointer into contribs and must not be freed directly.
 */
GList *
experiment_reader_get_contribution_by_time(GList *contribs, gint64 timept)
{
	for (GList *cur = contribs; cur != NULL; cur = cur->next) {
		ExperimentReaderContrib *contrib =
					(ExperimentReaderContrib *)cur->data;

		if (contrib->start_time > timept ||
		    cur->next == NULL)
			return cur;
	}

	return NULL;
}

/**
 * @brief Free list of contributions and associated data
 *
 * @sa experiment_reader_get_contributions_by_speaker
 *
 * @param contribs List of \ref ExperimentReaderContrib structures to free
 */
void
experiment_reader_free_contributions(GList *contribs)
{
	for (GList *cur = contribs; cur != NULL; cur = cur->next)
		g_free(cur->data);

	g_list_free(contribs);
}

/**
 * Calls \e callback with \e userdata for each \b topic in the \b greeting
 * section of the experiment.
 *
 * @param reader   \e ExperimentReader instance
 * @param callback Function to invoke
 * @param userdata User data to pass to \e callback
 */
void
experiment_reader_foreach_greeting_topic(ExperimentReader *reader,
					 ExperimentReaderTopicCallback callback,
					 gpointer userdata)
{
	xmlXPathContext	*xpathCtx;
	xmlXPathObject	*xpathObj;

	xpathCtx = xmlXPathNewContext(reader->priv->doc);
	xpathObj = xmlXPathEvalExpression(XML_CHAR("/session/greeting/topic"),
					  xpathCtx);

	generic_foreach_topic(reader, xpathObj->nodesetval,
			      callback, userdata);

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
}

/**
 * Calls \e callback with \e userdata for each \b topic in the
 * \b initial-narrative subsection of the \b experiment section of
 * the experiment.
 *
 * @param reader   \e ExperimentReader instance
 * @param callback Function to invoke
 * @param userdata User data to pass to \e callback
 */
void
experiment_reader_foreach_exp_initial_narrative_topic(reader, callback, userdata)
	ExperimentReader		*reader;
	ExperimentReaderTopicCallback	callback;
	gpointer			userdata;
{
	xmlXPathContext	*xpathCtx;
	xmlXPathObject	*xpathObj;

	xpathCtx = xmlXPathNewContext(reader->priv->doc);
	xpathObj = xmlXPathEvalExpression(XML_CHAR("/session/experiment/"
						   "initial-narrative/topic"),
					  xpathCtx);

	generic_foreach_topic(reader, xpathObj->nodesetval,
			      callback, userdata);

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
}

/**
 * Calls \e callback with \e userdata for each \b topic in a \b phase of
 * the \b last-minute subsection of the \b experiment section of
 * the experiment.
 *
 * @param reader   \e ExperimentReader instance
 * @param phase    \b Phase section (integer from 1 to 6)
 * @param callback Function to invoke
 * @param userdata User data to pass to \e callback
 */
void
experiment_reader_foreach_exp_last_minute_phase_topic(reader, phase, callback, userdata)
	ExperimentReader		*reader;
	gint				phase;
	ExperimentReaderTopicCallback	callback;
	gpointer			userdata;
{
	xmlXPathContext	*xpathCtx;
	xmlXPathObject	*xpathObj;

	xmlChar expr[255];

	xpathCtx = xmlXPathNewContext(reader->priv->doc);

	/* Evaluate xpath expression */
	xmlStrPrintf(expr, sizeof(expr),
		     XML_CHAR("/session/experiment/last-minute/"
			      "phase[@id = '%d']/topic"),
		     phase);
	xpathObj = xmlXPathEvalExpression(expr, xpathCtx);

	generic_foreach_topic(reader, xpathObj->nodesetval,
			      callback, userdata);

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
}

/**
 * Calls \e callback with \e userdata for each \b topic in the \b farewell
 * section of the experiment.
 *
 * @param reader   \e ExperimentReader instance
 * @param callback Function to invoke
 * @param userdata User data to pass to \e callback
 */
void
experiment_reader_foreach_farewell_topic(ExperimentReader *reader,
					 ExperimentReaderTopicCallback callback,
					 gpointer userdata)
{
	xmlXPathContext	*xpathCtx;
	xmlXPathObject	*xpathObj;

	xpathCtx = xmlXPathNewContext(reader->priv->doc);
	xpathObj = xmlXPathEvalExpression(XML_CHAR("/session/farewell/topic"),
					  xpathCtx);

	generic_foreach_topic(reader, xpathObj->nodesetval,
			      callback, userdata);

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
}
