#ifndef __EXPERIMENT_READER_H
#define __EXPERIMENT_READER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define EXPERIMENT_TYPE_READER \
	(experiment_reader_get_type())
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

typedef struct _ExperimentReaderPrivate ExperimentReaderPrivate;

typedef struct _ExperimentReader {
	GObject parent_instance;

	ExperimentReaderPrivate *priv; /** private */
} ExperimentReader;

typedef struct _ExperimentReaderClass {
	GObjectClass parent_class;
} ExperimentReaderClass;

GType experiment_reader_get_type(void);

/*
 * API
 */
ExperimentReader *experiment_reader_new(const gchar *filename);



G_END_DECLS

#endif
