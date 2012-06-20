/**
 * @file
 * Simple \e GtkVlcPlayer widget for playing media files via libVLC.
 * It supports fullscreen mode, a simple API, emitting signals on position and
 * length changes, as well as \e GtkAdjustment support for connecting the player
 * with scales and other widgets easily.
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

#include <assert.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#include <winuser.h>
#endif

#include <glib.h>
#include <glib/gprintf.h>

#include <gtk/gtk.h>
#ifdef G_OS_WIN32
#include <gdk/gdkwin32.h>
#else
#include <gdk/gdkx.h>
#endif

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "cclosure-marshallers.h"
#include "gtk-vlc-player.h"

static void gtk_vlc_player_class_init(GtkVlcPlayerClass *klass);
static inline libvlc_instance_t *create_vlc_instance(void);
static void gtk_vlc_player_init(GtkVlcPlayer *klass);

static void gtk_vlc_player_dispose(GObject *gobject);
static void gtk_vlc_player_finalize(GObject *gobject);

#ifdef G_OS_WIN32
static BOOL CALLBACK enumerate_vlc_windows_cb(HWND hWndvlc, LPARAM lParam);
static gboolean poll_vlc_event_window_cb(gpointer data);
#endif
static void widget_on_realize(GtkWidget *widget, gpointer data);
static gboolean widget_on_click(GtkWidget *widget, GdkEventButton *event, gpointer data);

static void time_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data);
static void time_adj_on_changed(GtkAdjustment *adj, gpointer user_data);
static void vol_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data);

static inline void update_time(GtkVlcPlayer *player, gint64 new_time);
static inline void update_length(GtkVlcPlayer *player, gint64 new_length);

static void vlc_time_changed(const struct libvlc_event_t *event, void *userdata);
static void vlc_length_changed(const struct libvlc_event_t *event, void *userdata);

static void vlc_player_load_media(GtkVlcPlayer *player, libvlc_media_t *media);

/** @private */
#define POLL_VLC_EVENT_WINDOW_INTERVAL 100 /* milliseconds */

/** @private */
#define GOBJECT_UNREF_SAFE(VAR) G_STMT_START {	\
	if ((VAR) != NULL) {			\
		g_object_unref(VAR);		\
		VAR = NULL;			\
	}					\
} G_STMT_END

/** @private */
#define GTK_VLC_PLAYER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_VLC_PLAYER, GtkVlcPlayerPrivate))

/** @private */
struct _GtkVlcPlayerPrivate {
	GtkObject		*time_adjustment;
	gulong			time_adj_on_value_changed_id;
	gulong			time_adj_on_changed_id;

	GtkObject		*volume_adjustment;
	gulong			vol_adj_on_value_changed_id;

	libvlc_instance_t	*vlc_inst;
	libvlc_media_player_t	*media_player;

	gboolean		isFullscreen;
	GtkWidget		*fullscreen_window;
};

/** @private */
enum {
	TIME_CHANGED_SIGNAL,
	LENGTH_CHANGED_SIGNAL,
	LAST_SIGNAL
};
static guint gtk_vlc_player_signals[LAST_SIGNAL] = {0, 0};

/**
 * @private
 * Will create \e gtk_vlc_player_get_type and set
 * \e gtk_vlc_player_parent_class
 */
G_DEFINE_TYPE(GtkVlcPlayer, gtk_vlc_player, GTK_TYPE_ALIGNMENT);

static void
gtk_vlc_player_class_init(GtkVlcPlayerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = gtk_vlc_player_dispose;
	gobject_class->finalize = gtk_vlc_player_finalize;

	gtk_vlc_player_signals[TIME_CHANGED_SIGNAL] =
		g_signal_new("time-changed",
			     G_TYPE_FROM_CLASS(klass),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GtkVlcPlayerClass, time_changed),
			     NULL, NULL,
			     gtk_vlc_player_marshal_VOID__INT64,
			     G_TYPE_NONE, 1, G_TYPE_INT64);

	gtk_vlc_player_signals[LENGTH_CHANGED_SIGNAL] =
		g_signal_new("length-changed",
			     G_TYPE_FROM_CLASS(klass),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GtkVlcPlayerClass, length_changed),
			     NULL, NULL,
			     gtk_vlc_player_marshal_VOID__INT64,
			     G_TYPE_NONE, 1, G_TYPE_INT64);

	g_type_class_add_private(klass, sizeof(GtkVlcPlayerPrivate));
}

static inline libvlc_instance_t *
create_vlc_instance(void)
{
	gchar	**vlc_argv;
	gint	vlc_argc = 0;

	libvlc_instance_t *ret;

	vlc_argv = g_malloc_n(2, sizeof(vlc_argv[0]));
	vlc_argv[vlc_argc++] = g_strdup(g_get_prgname());

#if LIBVLC_VERSION_INT < LIBVLC_VERSION(2,0,0,0)
	if (g_getenv("VLC_PLUGIN_PATH") != NULL) {
		vlc_argv = g_realloc_n(vlc_argv,
				       vlc_argc + 2 + 1, sizeof(vlc_argv[0]));
		vlc_argv[vlc_argc++] = g_strdup("--plugin-path");
		vlc_argv[vlc_argc++] = g_strdup(g_getenv("VLC_PLUGIN_PATH"));
	}
#endif

	vlc_argv[vlc_argc] = NULL;

	ret = libvlc_new((int)vlc_argc, (const char *const *)vlc_argv);

	g_strfreev(vlc_argv);
	return ret;
}

static void
gtk_vlc_player_init(GtkVlcPlayer *klass)
{
	GtkWidget		*drawing_area;
	GdkColor		color;
	libvlc_event_manager_t	*evman;

	klass->priv = GTK_VLC_PLAYER_GET_PRIVATE(klass);
	gtk_alignment_set(GTK_ALIGNMENT(klass), 0., 0., 1., 1.);

	drawing_area = gtk_drawing_area_new();
	/** @todo use styles */
	gdk_color_parse("black", &color);
	gtk_widget_modify_bg(drawing_area, GTK_STATE_NORMAL, &color);

	g_signal_connect(G_OBJECT(drawing_area), "realize",
			 G_CALLBACK(widget_on_realize), klass);

	gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(drawing_area), "button-press-event",
			 G_CALLBACK(widget_on_click), klass);

	gtk_container_add(GTK_CONTAINER(klass), drawing_area);
	gtk_widget_show(drawing_area);
	/*
	 * drawing area will be destroyed automatically with the
	 * GtkContainer/GtkVlcPlayer
	 * (it is derived from GtkObject and has one reference after adding it
	 * to the container).
	 */

	klass->priv->time_adjustment = gtk_adjustment_new(0., 0., 0.,
							  GTK_VLC_PLAYER_TIME_ADJ_STEP,
							  GTK_VLC_PLAYER_TIME_ADJ_PAGE,
							  0.);
	g_object_ref_sink(klass->priv->time_adjustment);
	klass->priv->time_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(klass->priv->time_adjustment),
				 "value-changed",
				 G_CALLBACK(time_adj_on_value_changed), klass);
	klass->priv->time_adj_on_changed_id =
		g_signal_connect(G_OBJECT(klass->priv->time_adjustment),
				 "changed",
				 G_CALLBACK(time_adj_on_changed), klass);

	klass->priv->volume_adjustment = gtk_adjustment_new(1., 0., 1.,
							    GTK_VLC_PLAYER_VOL_ADJ_STEP,
							    GTK_VLC_PLAYER_VOL_ADJ_PAGE,
							    0.);
	g_object_ref_sink(klass->priv->volume_adjustment);
	klass->priv->vol_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(klass->priv->volume_adjustment),
				 "value-changed",
				 G_CALLBACK(vol_adj_on_value_changed), klass);

	klass->priv->vlc_inst = create_vlc_instance();
	klass->priv->media_player = libvlc_media_player_new(klass->priv->vlc_inst);

	/* sign up for time updates */
	evman = libvlc_media_player_event_manager(klass->priv->media_player);

	libvlc_event_attach(evman, libvlc_MediaPlayerTimeChanged,
			    vlc_time_changed, klass);
	libvlc_event_attach(evman, libvlc_MediaPlayerLengthChanged,
			    vlc_length_changed, klass);

	klass->priv->isFullscreen = FALSE;
	klass->priv->fullscreen_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_object_ref_sink(klass->priv->fullscreen_window);

	gtk_window_set_deletable(GTK_WINDOW(klass->priv->fullscreen_window),
				 FALSE);
	gtk_window_set_decorated(GTK_WINDOW(klass->priv->fullscreen_window),
				 FALSE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(klass->priv->fullscreen_window),
					 TRUE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(klass->priv->fullscreen_window),
				       TRUE);
}

static void
gtk_vlc_player_dispose(GObject *gobject)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(gobject);

	/*
	 * destroy might be called more than once, but we have only one
	 * reference for each object
	 */
	if (player->priv->time_adjustment != NULL) {
		g_signal_handler_disconnect(G_OBJECT(player->priv->time_adjustment),
					    player->priv->time_adj_on_changed_id);
		g_signal_handler_disconnect(G_OBJECT(player->priv->time_adjustment),
					    player->priv->time_adj_on_value_changed_id);
		g_object_unref(player->priv->time_adjustment);
		player->priv->time_adjustment = NULL;
	}
	if (player->priv->volume_adjustment != NULL) {
		g_signal_handler_disconnect(G_OBJECT(player->priv->volume_adjustment),
					    player->priv->vol_adj_on_value_changed_id);
		g_object_unref(player->priv->volume_adjustment);
		player->priv->volume_adjustment = NULL;
	}
	GOBJECT_UNREF_SAFE(player->priv->fullscreen_window);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(gtk_vlc_player_parent_class)->dispose(gobject);
}

static void
gtk_vlc_player_finalize(GObject *gobject)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(gobject);

	libvlc_media_player_release(player->priv->media_player);
	libvlc_release(player->priv->vlc_inst);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(gtk_vlc_player_parent_class)->finalize(gobject);
}

#ifdef G_OS_WIN32

static BOOL CALLBACK
enumerate_vlc_windows_cb(HWND hWndvlc, LPARAM lParam)
{
	EnableWindow(hWndvlc, FALSE);
	*(gboolean *)lParam = FALSE;

	return TRUE;
}

/**
 * @brief Callback for polling the availability of a libVLC event window.
 *
 * If found, it is disabled and the polling stops. This results in mouse click
 * events being delivered to the GtkDrawingArea widget.
 *
 * @param data User data (\e GtkVlcPlayer widget)
 * @return \c TRUE to continue polling, \c FALSE to stop
 */
static gboolean
poll_vlc_event_window_cb(gpointer data)
{
	GtkWidget *drawing_area = gtk_bin_get_child(GTK_BIN(data));
	GdkWindow *window = gtk_widget_get_window(drawing_area);

	gboolean ret = TRUE;

	EnumChildWindows(GDK_WINDOW_HWND(window),
			 enumerate_vlc_windows_cb, (LPARAM)&ret);

	return ret;
}

static void
widget_on_realize(GtkWidget *widget, gpointer user_data)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(user_data);
	GdkWindow *window = gtk_widget_get_window(widget);

	libvlc_media_player_set_hwnd(player->priv->media_player,
				     GDK_WINDOW_HWND(window));
}

#else

static void
widget_on_realize(GtkWidget *widget, gpointer user_data)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(user_data);
	GdkWindow *window = gtk_widget_get_window(widget);

	libvlc_media_player_set_xwindow(player->priv->media_player,
					GDK_WINDOW_XID(window));
}

#endif

static gboolean
widget_on_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(user_data);
	GtkWidget *fullscreen_window = player->priv->fullscreen_window;

	if (fullscreen_window == NULL)
		return TRUE;

	if (event->type != GDK_2BUTTON_PRESS || event->button != 1)
		return TRUE;

	if (player->priv->isFullscreen) {
		gtk_widget_reparent(widget, GTK_WIDGET(player));
		gtk_widget_show(widget);
		gtk_window_unfullscreen(GTK_WINDOW(fullscreen_window));
		gtk_widget_hide(fullscreen_window);

		player->priv->isFullscreen = FALSE;
	} else {
		gtk_window_fullscreen(GTK_WINDOW(fullscreen_window));
		gtk_widget_show(fullscreen_window);
		gtk_widget_reparent(widget, fullscreen_window);
		gtk_widget_show(widget);

		player->priv->isFullscreen = TRUE;
	}

	return TRUE;
}

static void
time_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data)
{
	gtk_vlc_player_seek(GTK_VLC_PLAYER(user_data),
			    (gint64)gtk_adjustment_get_value(adj));
}

static void
time_adj_on_changed(GtkAdjustment *adj, gpointer user_data)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(user_data);

	/* avoid signal handler recursion */
	g_signal_handler_block(G_OBJECT(adj),
			       player->priv->time_adj_on_changed_id);
	gtk_adjustment_set_upper(adj, (gdouble)gtk_vlc_player_get_length(player) +
				      gtk_adjustment_get_page_size(adj));
	g_signal_handler_unblock(G_OBJECT(adj),
				 player->priv->time_adj_on_changed_id);
}

static void
vol_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data)
{
	gtk_vlc_player_set_volume(GTK_VLC_PLAYER(user_data),
				  gtk_adjustment_get_value(adj));
}

static inline void
update_time(GtkVlcPlayer *player, gint64 new_time)
{
	g_signal_emit(player, gtk_vlc_player_signals[TIME_CHANGED_SIGNAL], 0,
		      new_time);

	if (player->priv->time_adjustment != NULL) {
		/* ensure that time_adj_on_value_changed() will not be executed */
		g_signal_handler_block(G_OBJECT(player->priv->time_adjustment),
				       player->priv->time_adj_on_value_changed_id);
		gtk_adjustment_set_value(GTK_ADJUSTMENT(player->priv->time_adjustment),
					 (gdouble)new_time);
		g_signal_handler_unblock(G_OBJECT(player->priv->time_adjustment),
					 player->priv->time_adj_on_value_changed_id);
	}
}

static inline void
update_length(GtkVlcPlayer *player, gint64 new_length)
{
	g_signal_emit(player, gtk_vlc_player_signals[LENGTH_CHANGED_SIGNAL], 0,
		      new_length);

	if (player->priv->time_adjustment != NULL) {
		GtkAdjustment *adj = GTK_ADJUSTMENT(player->priv->time_adjustment);

		/* ensure that time_adj_on_changed() will not be executed */
		g_signal_handler_block(G_OBJECT(adj),
				       player->priv->time_adj_on_changed_id);
		gtk_adjustment_set_upper(adj, (gdouble)new_length +
					      gtk_adjustment_get_page_size(adj));
		g_signal_handler_unblock(G_OBJECT(adj),
					 player->priv->time_adj_on_changed_id);
	}
}

static void
vlc_time_changed(const struct libvlc_event_t *event, void *user_data)
{
	assert(event->type == libvlc_MediaPlayerTimeChanged);

	/* VLC callbacks are invoked from another thread! */
	gdk_threads_enter();
	update_time(GTK_VLC_PLAYER(user_data),
		    (gint64)event->u.media_player_time_changed.new_time);
	gdk_threads_leave();
}

static void
vlc_length_changed(const struct libvlc_event_t *event, void *user_data)
{
	assert(event->type == libvlc_MediaPlayerLengthChanged);

	/* VLC callbacks are invoked from another thread! */
	gdk_threads_enter();
	update_length(GTK_VLC_PLAYER(user_data),
		      (gint64)event->u.media_player_length_changed.new_length);
    	gdk_threads_leave();
}

static void
vlc_player_load_media(GtkVlcPlayer *player, libvlc_media_t *media)
{
	libvlc_media_parse(media);
	libvlc_media_player_set_media(player->priv->media_player, media);

	/* NOTE: media was parsed so get_duration works */
	update_length(player, (gint64)libvlc_media_get_duration(media));
	update_time(player, 0);
}

/*
 * API
 */

/**
 * @brief Construct new \e GtkVlcPlayer widget instance.
 *
 * @return New \e GtkVlcPlayer widget instance
 */
GtkWidget *
gtk_vlc_player_new(void)
{
	return GTK_WIDGET(g_object_new(GTK_TYPE_VLC_PLAYER, NULL));
}

/**
 * @brief Load media with specified filename into player widget
 *
 * It does not start playing until playback is started or toggled.
 * "time-changed" and "length-changed" signals will be emitted immediately
 * after successfully loading the media. The time-adjustment will also be
 * reconfigured appropriately.
 *
 * @param player \e GtkVlcPlayer instance to load file into.
 * @param file   \e Filename to load
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_vlc_player_load_filename(GtkVlcPlayer *player, const gchar *file)
{
	libvlc_media_t *media;

	media = libvlc_media_new_path(player->priv->vlc_inst,
				      (const char *)file);
	if (media == NULL)
		return FALSE;
	vlc_player_load_media(player, media);
	libvlc_media_release(media);

	return TRUE;
}

/**
 * @brief Load media with specified URI into player widget
 *
 * It is otherwise identical to \ref gtk_vlc_player_load_filename.
 *
 * @sa gtk_vlc_player_load_filename
 *
 * @param player \e GtkVlcPlayer instance to load media into.
 * @param uri    \e URI to load
 * @return \c TRUE on success, else \c FALSE
 */
gboolean
gtk_vlc_player_load_uri(GtkVlcPlayer *player, const gchar *uri)
{
	libvlc_media_t *media;

	media = libvlc_media_new_location(player->priv->vlc_inst,
					  (const char *)uri);
	if (media == NULL)
		return FALSE;
	vlc_player_load_media(player, media);
	libvlc_media_release(media);

	return TRUE;
}

/**
 * @brief Play back media if playback is currently paused
 *
 * If it is already playing, do nothing.
 * In playback mode, there will be constant "time-changed" signal emissions
 * and the time-adjustment's value will be set accordingly.
 *
 * @param player \e GtkVlcPlayer instance
 */
void
gtk_vlc_player_play(GtkVlcPlayer *player)
{
	if (libvlc_media_player_play(player->priv->media_player) < 0)
		return;

	/*
	 * Workaround to get mouse click events on the drawing area widget
	 * that provides the low-level window for libVLC.
	 * On Win32, libVLC creates an event window (in a different thread)
	 * after playback start that "swallows" all click events.
	 * So we have to poll for the availability of that window and disable
	 * it.
	 */
#ifdef G_OS_WIN32
	g_timeout_add(POLL_VLC_EVENT_WINDOW_INTERVAL,
		      poll_vlc_event_window_cb, player);
#endif
}

/**
 * @brief Pause media playback if it is currently playing
 *
 * If it is already paused, do nothing.
 *
 * @param player \e GtkVlcPlayer instance
 */
void
gtk_vlc_player_pause(GtkVlcPlayer *player)
{
	libvlc_media_player_pause(player->priv->media_player);
}

/**
 * @brief Toggle media playback
 *
 * If it is currently playing, pause playback. If it is currently paused,
 * start playback.
 *
 * @sa gtk_vlc_player_play
 * @sa gtk_vlc_player_pause
 *
 * @param player \e GtkVlcPlayer instance
 * @return \c TRUE if media is playing \b after the operation,
 *         \c FALSE if it is paused.
 */
gboolean
gtk_vlc_player_toggle(GtkVlcPlayer *player)
{
	if (libvlc_media_player_is_playing(player->priv->media_player))
		gtk_vlc_player_pause(player);
	else
		gtk_vlc_player_play(player);

	return (gboolean)libvlc_media_player_is_playing(player->priv->media_player);
}

/**
 * @brief Stop media playback
 *
 * A "time-changed" signal will be emitted and the time-adjustment will be
 * reset appropriately.
 *
 * @param player \e GtkVlcPlayer instance
 */
void
gtk_vlc_player_stop(GtkVlcPlayer *player)
{
	gtk_vlc_player_pause(player);
	libvlc_media_player_stop(player->priv->media_player);

	update_time(player, 0);
}

/**
 * @brief Set point of time in playback
 *
 * @param player \e GtkVlcPlayer instance
 * @param time   New position in media (milliseconds)
 */
void
gtk_vlc_player_seek(GtkVlcPlayer *player, gint64 time)
{
	libvlc_media_player_set_time(player->priv->media_player,
				     (libvlc_time_t)time);
}

/**
 * @brief Set audio volume of playback
 *
 * @param player \e GtkVlcPlayer instance
 * @param volume New volume of playback. It must be between 0.0 (0%) and 2.0 (200%)
 *               of the original audio volume.
 */
void
gtk_vlc_player_set_volume(GtkVlcPlayer *player, gdouble volume)
{
	libvlc_audio_set_volume(player->priv->media_player, (int)(volume*100.));
}

/**
 * @brief Get media length
 *
 * @param player \e GtkVlcPlayer instance
 * @return Length of media (in milliseconds)
 */
gint64
gtk_vlc_player_get_length(GtkVlcPlayer *player)
{
	return (gint64)libvlc_media_player_get_length(player->priv->media_player);
}

/**
 * @brief Get time-adjustment currently used by \e GtkVlcPlayer
 *
 * The time-adjustment is an alternative to signal-callbacks and using the API
 * for synchronizing the \e GtkVlcPlayer widget's current playback position with
 * another widget (e.g. a \e GtkScale).
 * The adjustment's value is in milliseconds.
 * The widget will initialize one on construction, so there \e should always be
 * an adjustment to get.
 *
 * @sa gtk_vlc_player_seek
 * @sa GtkVlcPlayerClass::time_changed GtkVlcPlayerClass::length_changed
 *
 * @param player \e GtkVlcPlayer instance
 * @return Currently used time-adjustment
 */
GtkAdjustment *
gtk_vlc_player_get_time_adjustment(GtkVlcPlayer *player)
{
	return player->priv->time_adjustment != NULL
			? GTK_ADJUSTMENT(player->priv->time_adjustment)
			: NULL;
}

/**
 * @brief Change time-adjustment used by \e GtkVlcPlayer
 *
 * The old adjustment will be
 * unreferenced (and possibly destroyed) and a reference to the new
 * adjustment will be fetched.
 *
 * @sa gtk_vlc_player_get_time_adjustment
 *
 * @param player \e GtkVlcPlayer instance
 * @param adj    New \e GtkAdjustment to use as time-adjustment.
 */
void
gtk_vlc_player_set_time_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj)
{
	if (player->priv->time_adjustment == NULL)
		return;

	g_signal_handler_disconnect(G_OBJECT(player->priv->time_adjustment),
				    player->priv->time_adj_on_changed_id);
	g_signal_handler_disconnect(G_OBJECT(player->priv->time_adjustment),
				    player->priv->time_adj_on_value_changed_id);

	g_object_unref(player->priv->time_adjustment);
	player->priv->time_adjustment = GTK_OBJECT(adj);
	g_object_ref_sink(player->priv->time_adjustment);

	/*
	 * NOTE: not setting value and upper properly, as well as setting the
	 * page-size might cause problems if adjustment is set after loading/playing
	 * a media or setting it on other widgets
	 */
	gtk_adjustment_configure(GTK_ADJUSTMENT(player->priv->time_adjustment),
				 0., 0., 0.,
				 GTK_VLC_PLAYER_TIME_ADJ_STEP,
				 GTK_VLC_PLAYER_TIME_ADJ_PAGE,
				 0.);

	player->priv->time_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(player->priv->time_adjustment),
				 "value-changed",
				 G_CALLBACK(time_adj_on_value_changed), player);
	player->priv->time_adj_on_changed_id =
		g_signal_connect(G_OBJECT(player->priv->time_adjustment),
				 "changed",
				 G_CALLBACK(time_adj_on_changed), player);
}

/**
 * @brief Get volume-adjustment currently used by \e GtkVlcPlayer
 *
 * The volume-adjustment is an alternative to signal-callbacks and using the
 * API for synchronizing the \e GtkVlcPlayer widget's current playback volume
 * with another widget (e.g. a \e GtkVolumeButton).
 * The adjustment's value is a percentage of gain to apply to the original
 * signal (0.0 is 0%, 2.0 is 200%).
 * The widget will initialize a volume-adjustment on construction, so there will
 * always be an adjustment to get.
 * In contrast to the time-adjustment, the volume-adjustment's value will never
 * be changed by the \e GtkVlcPlayer widget.
 *
 * @sa gtk_vlc_player_set_volume
 *
 * @param player \e GtkVlcPlayer instance
 * @return Currently used volume-adjustment
 */
GtkAdjustment *
gtk_vlc_player_get_volume_adjustment(GtkVlcPlayer *player)
{
	return player->priv->volume_adjustment != NULL
			? GTK_ADJUSTMENT(player->priv->volume_adjustment)
			: NULL;
}

/**
 * @brief Change volume-adjustment used by \e GtkVlcPlayer
 *
 * The old adjustment will be unreferenced (and possibly destroyed) and a
 * reference to the new adjustment will be fetched.
 *
 * @sa gtk_vlc_player_get_volume_adjustment
 *
 * @param player \e GtkVlcPlayer instance
 * @param adj    New \e GtkAdjustment to use as volume-adjustment.
 */
void
gtk_vlc_player_set_volume_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj)
{
	if (player->priv->volume_adjustment == NULL)
		return;

	g_signal_handler_disconnect(G_OBJECT(player->priv->volume_adjustment),
				    player->priv->vol_adj_on_value_changed_id);

	g_object_unref(player->priv->volume_adjustment);
	player->priv->volume_adjustment = GTK_OBJECT(adj);
	g_object_ref_sink(player->priv->volume_adjustment);

	gtk_adjustment_configure(GTK_ADJUSTMENT(player->priv->volume_adjustment),
				 1., 0., 1.,
				 GTK_VLC_PLAYER_VOL_ADJ_STEP,
				 GTK_VLC_PLAYER_VOL_ADJ_PAGE,
				 0.);

	player->priv->vol_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(player->priv->volume_adjustment),
				 "value-changed",
				 G_CALLBACK(vol_adj_on_value_changed), player);
}
