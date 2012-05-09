#ifndef __EXPERIMENT_PLAYER_H
#define __EXPERIMENT_PLAYER_H

#include <gtk/gtk.h>

gboolean load_media_file(const gchar *file);
void show_message_dialog_gerror(GError *err);

extern GtkWidget *player_window;

extern GtkWidget *player_widget,
		 *controls_hbox,
		 *scale_widget,
		 *playpause_button,
		 *volume_button;

extern gchar *current_filename;

void refresh_quickopen_menu(GtkMenu *menu);

extern GtkWidget *quickopen_menu,
		 *quickopen_menu_empty_item;

extern gchar *quickopen_directory;

#define BUILDER_INIT(BUILDER, VAR) do {					\
	VAR = GTK_WIDGET(gtk_builder_get_object(BUILDER, #VAR));	\
} while (0)

#endif