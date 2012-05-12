/**
 * @file
 * Header file necessary to include when using the \e GtkVlcPlayer
 * widget.
 */

#ifndef __GTK_VLC_PLAYER_H
#define __GTK_VLC_PLAYER_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_VLC_PLAYER \
	(gtk_vlc_player_get_type())
/**
 * Cast instance pointer to \e GtkVlcPlayer
 *
 * @param obj Object to cast to \e GtkVlcPlayer
 */
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

/** @private */
typedef struct _GtkVlcPlayerPrivate GtkVlcPlayerPrivate;

/**
 * \e GtkVlcPlayer instance structure
 */
typedef struct _GtkVlcPlayer {
	GtkAlignment parent_instance;	/**< Parent instance structure */

	GtkVlcPlayerPrivate *priv;	/**< @private */
} GtkVlcPlayer;

/**
 * \e GtkExperimentNavigator class structure
 */
typedef struct _GtkVlcPlayerClass {
	GtkAlignmentClass parent_class;	/**< Parent class structure */

	/**
	 * Callback function to invoke when emitting the "time-changed"
	 * signal. Do not set manually.
	 */
	void (*time_changed)	(GtkVlcPlayer *self, gint64 new_time, gpointer user_data);

	/**
	 * Callback function to invoke when emitting the "length-changed"
	 * signal. Do not set manually.
	 */
	void (*length_changed)	(GtkVlcPlayer *self, gint64 new_length, gpointer user_data);
} GtkVlcPlayerClass;

/** @private */
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
