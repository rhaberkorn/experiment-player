/**
 * @file
 * Header file necessary to include when using the \e GtkVlcPlayer
 * widget.
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
 * @return \e obj casted to \e GtkVlcPlayer
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
	 * signal.
	 *
	 * @param self      \e GtkVlcPlayer widget that emitted the signal
	 * @param new_time  New (current) position of playback in milliseconds
	 */
	void (*time_changed)	(GtkVlcPlayer *self, gint64 new_time);

	/**
	 * Callback function to invoke when emitting the "length-changed"
	 * signal.
	 *
	 * @param self       \e GtkVlcPlayer widget that emitted the signal
	 * @param new_length New (current) length of media loaded into player (milliseconds)
	 */
	void (*length_changed)	(GtkVlcPlayer *self, gint64 new_length);
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

gint64 gtk_vlc_player_get_length(GtkVlcPlayer *player);

GtkAdjustment *gtk_vlc_player_get_time_adjustment(GtkVlcPlayer *player);
void gtk_vlc_player_set_time_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj);

GtkAdjustment *gtk_vlc_player_get_volume_adjustment(GtkVlcPlayer *player);
void gtk_vlc_player_set_volume_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj);

G_END_DECLS

#endif
