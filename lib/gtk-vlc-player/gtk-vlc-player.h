#ifndef __GTK_VLC_PLAYER_H
#define __GTK_VLC_PLAYER_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include <vlc/vlc.h>

G_BEGIN_DECLS

#define GTK_TYPE_VLC_PLAYER \
	(gtk_vlc_player_get_type())
#define GTK_VLC_PLAYER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_VLC_PLAYER, GtkVlcPlayer))
#define GTK_VLC_PLAYER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_VLC_PLAYER, GtkVlcPlayerClass))
#define GTK_IS_VLC_PLAYER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_VLC_PLAYER))
#define GTK_IS_VLC_PLAYER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_VLC_PLAYER))
#define GTK_VLC_PLAYER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_VLC_PLAYER, GtkVlcPlayerClass))

typedef struct _GtkVlcPlayer {
	GtkAlignment parent_instance;

	GtkObject		*time_adjustment;
	gulong			time_adj_on_value_changed_id;

	GtkObject		*volume_adjustment;
	gulong			vol_adj_on_value_changed_id;

	libvlc_instance_t	*vlc_inst;
	libvlc_media_player_t	*media_player;

	gboolean		isFullscreen;
	GtkWidget		*fullscreen_window;
} GtkVlcPlayer;

typedef struct _GtkVlcPlayerClass {
	GtkAlignmentClass parent_class;

	void (*time_changed)	(GtkVlcPlayer *self, gint64 new_time, gpointer user_data);
	void (*length_changed)	(GtkVlcPlayer *self, gint64 new_length, gpointer user_data);
} GtkVlcPlayerClass;

GType gtk_vlc_player_get_type(void);

/*
 * API
 */
GtkWidget *gtk_vlc_player_new(void);

gboolean gtk_vlc_player_load_filename(GtkVlcPlayer *player, const gchar *file);
gboolean gtk_vlc_player_load_uri(GtkVlcPlayer *player, const gchar *uri);

void gtk_vlc_player_play(GtkVlcPlayer *player);
void gtk_vlc_player_pause(GtkVlcPlayer *player);
gboolean gtk_vlc_player_toggle(GtkVlcPlayer *player);
void gtk_vlc_player_stop(GtkVlcPlayer *player);

void gtk_vlc_player_seek(GtkVlcPlayer *player, gint64 time);
void gtk_vlc_player_set_volume(GtkVlcPlayer *player, gdouble volume);

GtkAdjustment *gtk_vlc_player_get_time_adjustment(GtkVlcPlayer *player);
void gtk_vlc_player_set_time_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj);

GtkAdjustment *gtk_vlc_player_get_volume_adjustment(GtkVlcPlayer *player);
void gtk_vlc_player_set_volume_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj);

G_END_DECLS

#endif
