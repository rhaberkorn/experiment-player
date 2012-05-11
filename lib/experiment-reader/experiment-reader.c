#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#define EXPERIMENT_READER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), EXPERIMENT_TYPE_READER, ExperimentReaderPrivate))

struct _ExperimentReaderPrivate {
	xmlDoc *doc;
};

GType
experiment_reader_get_type(void)
{
	static GType type = 0;

	if (!type) {
		const GTypeInfo info = {
			sizeof(ExperimentReaderClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)experiment_reader_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(ExperimentReader),
			0,    /* n_preallocs */
			(GInstanceInitFunc)experiment_reader_init,
		};

		type = g_type_register_static(G_TYPE_OBJECT,
					      "ExperimentReader", &info, 0);
	}

	return type;
}

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
	//G_OBJECT_CLASS(experiment_reader_parent_class)->finalize(gobject);
}

/*
 * API
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

	/* TODO: validate against session.dtd */

	return reader;
}

#if 0
void
experiment_reader_greeting_foreach_topic(ExperimentReader *reader,
					 ExperimentReaderTopicCb cb,
					 gpointer data)
{
	
}
#endif