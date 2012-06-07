/**
 * @file
 * libexperiment-reader unit tests using GTester
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

#include <inttypes.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <experiment-reader.h>

#define TEST_EXPERIMENT_VALID	"test-experiment-valid.xml"
/* #define TEST_EXPERIMENT_INVALID "test-experiment-invalid.xml" */

static void
test_new_valid(void)
{
	ExperimentReader *reader;

	reader = experiment_reader_new(TEST_EXPERIMENT_VALID);
	g_assert(reader != NULL);

	g_object_unref(reader);
}

static void
test_foreach_greeting_topic_values_cb(ExperimentReader *reader,
				      const gchar *topic_id,
				      gint64 start_time,
				      gpointer data)
{
	gint *i = (gint *)data;

	g_assert(reader != NULL);
	g_assert(data != NULL);

	switch (*i) {
	case 0:
		g_assert_cmpstr(topic_id, ==, "bz_2");
		g_assert_cmpint(start_time, ==, 13648);
		break;
	default:
		g_assert_not_reached();
	}

	++*i;
}

static void
test_foreach_greeting_topic_values(void)
{
	ExperimentReader *reader;
	gint cb_iter = 0;

	reader = experiment_reader_new(TEST_EXPERIMENT_VALID);
	g_assert(reader != NULL);

	experiment_reader_foreach_greeting_topic(reader,
						 test_foreach_greeting_topic_values_cb,
						 &cb_iter);

	g_object_unref(reader);
}

/** @private */
int
main(int argc, char **argv)
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/api/new/test_valid", test_new_valid);

	g_test_add_func("/api/foreach_greeting_topic/test_values",
			test_foreach_greeting_topic_values);

	g_test_run_suite(g_test_get_root());

	return 0;
}
