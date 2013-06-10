/**
 * @file
 * Header file to include when using the \e ExperimentReader class.
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

#ifndef __EXPERIMENT_READER_H
#define __EXPERIMENT_READER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define EXPERIMENT_TYPE_READER \
	(experiment_reader_get_type())
/**
 * Cast instance pointer to \e ExperimentReader
 *
 * @param obj Object to cast to \e ExperimentReader
 * @return \e obj casted to \e ExperimentReader
 */
#define EXPERIMENT_READER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), EXPERIMENT_TYPE_READER, ExperimentReader))
#define EXPERIMENT_READER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), EXPERIMENT_TYPE_READER, ExperimentReaderClass))
#define EXPERIMENT_IS_READER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), EXPERIMENT_TYPE_READER))
#define EXPERIMENT_IS_READER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), EXPERIMENT_TYPE_READER))
#define EXPERIMENT_READER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), EXPERIMENT_TYPE_READER, ExperimentReaderClass))

/** @private */
typedef struct _ExperimentReaderPrivate ExperimentReaderPrivate;

/**
 * \e ExperimentReader instance structure
 */
typedef struct _ExperimentReader {
	GObject parent_instance;	/**< Parent instance structure */

	ExperimentReaderPrivate *priv;	/**< @private */
} ExperimentReader;

/**
 * \e ExperimentReader class structure
 */
typedef struct _ExperimentReaderClass {
	GObjectClass parent_class;	/**< Parent class structure */
} ExperimentReaderClass;

/** @private */
GType experiment_reader_get_type(void);

/*
 * Callbacks
 */
/**
 * Type of function to use for \e topic callbacks.
 *
 * @param reader     \e ExperimentReader the information refers to
 * @param topic_id   Symbolic identifier of experiment \b topic
 * @param start_time Beginning of first \b contribution in \e topic (milliseconds)
 * @param end_time   End of last \b contribution in \e topic (milliseconds)
 * @param data       Callback user data
 */
typedef void (*ExperimentReaderTopicCallback)(ExperimentReader *reader,
					      const gchar *topic_id,
					      gint64 start_time,
					      gint64 end_time,
					      gpointer data);

/**
 * Structure describing a contribution. Every text-fragment identified by
 * a distinct \e timepoint is considered a contribution.
 */
typedef struct {
	gint64	start_time;	/**< Contribution's start time in milliseconds */
	gchar	text[];		/**< Contribution's text content (part of the structure) */
} ExperimentReaderContrib;

/*
 * API
 */
ExperimentReader *experiment_reader_new(const gchar *filename);

GList *experiment_reader_get_contributions_by_speaker(
	ExperimentReader		*reader,
	const gchar			*speaker);
GList *experiment_reader_get_contribution_by_time(
	GList				*contribs,
	gint64				timept);
void experiment_reader_free_contributions(
	GList				*contribs);

void experiment_reader_foreach_greeting_topic(
	ExperimentReader		*reader,
	ExperimentReaderTopicCallback	callback,
	gpointer			userdata);
void experiment_reader_foreach_exp_initial_narrative_topic(
	ExperimentReader		*reader,
	ExperimentReaderTopicCallback	callback,
	gpointer			userdata);
void experiment_reader_foreach_exp_last_minute_phase_topic(
	ExperimentReader		*reader,
	gint				phase,
	ExperimentReaderTopicCallback	callback,
	gpointer			userdata);
void experiment_reader_foreach_farewell_topic(
	ExperimentReader		*reader,
	ExperimentReaderTopicCallback	callback,
	gpointer			userdata);

G_END_DECLS

#endif
