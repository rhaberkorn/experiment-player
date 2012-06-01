#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include "experiment-player.h"

static GKeyFile	*keyfile;
static gchar	*filename = NULL;

void
config_init_key_file(void)
{
	keyfile = g_key_file_new();

	/* initialize defaults */
	config_set_quickopen_directory(DEFAULT_QUICKOPEN_DIR);
	config_set_formats_directory(DEFAULT_FORMATS_DIR);

	/* may fail if no serialized configuration exists */
	g_key_file_load_from_data_dirs(keyfile, CONFIG_KEY_FILE, &filename,
				       G_KEY_FILE_KEEP_COMMENTS, NULL);

	if (filename == NULL)
		filename = g_build_filename(g_get_user_data_dir(),
					    CONFIG_KEY_FILE, NULL);
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
