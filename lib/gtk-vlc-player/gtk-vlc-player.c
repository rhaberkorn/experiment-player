#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <gtk/gtk.h>
#ifdef __WIN32__
#include <gdk/gdkwin32.h>
#else
#include <gdk/gdkx.h>
#endif

#include <vlc/vlc.h>

#include "cclosure-marshallers.h"
#include "gtk-vlc-player.h"

static void gtk_vlc_player_class_init(GtkVlcPlayerClass *klass);
static void gtk_vlc_player_init(GtkVlcPlayer *klass);

static void gtk_vlc_player_dispose(GObject *gobject);
static void gtk_vlc_player_finalize(GObject *gobject);

static void widget_on_realize(GtkWidget *widget, gpointer data);
static gboolean widget_on_click(GtkWidget *widget, GdkEventButton *event, gpointer data);

static void time_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data);
static void vol_adj_on_value_changed(GtkAdjustment *adj, gpointer user_data);

static inline void update_time(GtkVlcPlayer *player, gint64 new_time);
static inline void update_length(GtkVlcPlayer *player, gint64 new_length);

static void vlc_time_changed(const struct libvlc_event_t *event, void *userdata);
static void vlc_length_changed(const struct libvlc_event_t *event, void *userdata);

static void vlc_player_load_media(GtkVlcPlayer *player, libvlc_media_t *media);

/** @private */
#define GOBJECT_UNREF_SAFE(VAR) do {	\
	if ((VAR) != NULL) {		\
		g_object_unref(VAR);	\
		VAR = NULL;		\
	}				\
} while (0)

/** @private */
#define GTK_VLC_PLAYER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_VLC_PLAYER, GtkVlcPlayerPrivate))

/** @private */
struct _GtkVlcPlayerPrivate {
	GtkObject		*time_adjustment;
	gulong			time_adj_on_value_changed_id;

	GtkObject		*volume_adjustment;
	gulong			vol_adj_on_value_changed_id;

	libvlc_instance_t	*vlc_inst;
	libvlc_media_player_t	*media_player;

	gboolean		isFullscreen;
	GtkWidget		*fullscreen_window;
};

enum {
	TIME_CHANGED_SIGNAL,
	LENGTH_CHANGED_SIGNAL,
	LAST_SIGNAL
};
static guint gtk_vlc_player_signals[LAST_SIGNAL] = {0, 0};

/**
 * @private
 * will create gtk_vlc_player_get_type and set gtk_vlc_player_parent_class
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
							  GTK_VLC_PLAYER_ADJ_STEP,
							  GTK_VLC_PLAYER_ADJ_PAGE, 0.);
	g_object_ref_sink(klass->priv->time_adjustment);
	klass->priv->time_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(klass->priv->time_adjustment),
				 "value-changed",
				 G_CALLBACK(time_adj_on_value_changed), klass);

	klass->priv->volume_adjustment = gtk_adjustment_new(1., 0., 1.,
							    0.02, 0., 0.);
	g_object_ref_sink(klass->priv->volume_adjustment);
	klass->priv->vol_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(klass->priv->volume_adjustment),
				 "value-changed",
				 G_CALLBACK(vol_adj_on_value_changed), klass);

	klass->priv->vlc_inst = libvlc_new(0, NULL);
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
}

static void
gtk_vlc_player_dispose(GObject *gobject)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(gobject);

	/*
	 * destroy might be called more than once, but we have only one
	 * reference for each object
	 */
	GOBJECT_UNREF_SAFE(player->priv->time_adjustment);
	GOBJECT_UNREF_SAFE(player->priv->volume_adjustment);
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

static void
widget_on_realize(GtkWidget *widget, gpointer user_data)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(user_data);
	GdkWindow *window = gtk_widget_get_window(widget);

#ifdef __WIN32__
	libvlc_media_player_set_hwnd(player->priv->media_player,
				     GDK_WINDOW_HWND(window));
#else
	libvlc_media_player_set_xwindow(player->priv->media_player,
					GDK_WINDOW_XID(window));
#endif
}

/**
 * @bug
 * We don't get the signal on Windows (MinGW), after a movie starts playing.
 * Using GtkEventBoxes does \b NOT help.
 */
static gboolean
widget_on_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkVlcPlayer *player = GTK_VLC_PLAYER(user_data);

	if (event->type != GDK_2BUTTON_PRESS || event->button != 1)
		return TRUE;

	if (player->priv->isFullscreen) {
		gtk_widget_reparent(widget, GTK_WIDGET(player));
		gtk_widget_show(widget);
		gtk_widget_hide(player->priv->fullscreen_window);
		gtk_window_unfullscreen(GTK_WINDOW(player->priv->fullscreen_window));

		player->priv->isFullscreen = FALSE;
	} else {
		gtk_window_fullscreen(GTK_WINDOW(player->priv->fullscreen_window));
		gtk_widget_show(player->priv->fullscreen_window);
		gtk_widget_reparent(widget, player->priv->fullscreen_window);
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

	/* ensure that time_adj_on_value_changed() will not be executed */
	g_signal_handler_block(G_OBJECT(player->priv->time_adjustment),
			       player->priv->time_adj_on_value_changed_id);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(player->priv->time_adjustment),
				 (gdouble)new_time);
	g_signal_handler_unblock(G_OBJECT(player->priv->time_adjustment),
				 player->priv->time_adj_on_value_changed_id);
}

static inline void
update_length(GtkVlcPlayer *player, gint64 new_length)
{
	g_signal_emit(player, gtk_vlc_player_signals[LENGTH_CHANGED_SIGNAL], 0,
		      new_length);

	gtk_adjustment_set_upper(GTK_ADJUSTMENT(player->priv->time_adjustment),
				 (gdouble)new_length);
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

GtkWidget *
gtk_vlc_player_new(void)
{
	return GTK_WIDGET(g_object_new(GTK_TYPE_VLC_PLAYER, NULL));
}

gboolean
gtk_vlc_player_load_filename(GtkVlcPlayer *player, const gchar *file)
{
	libvlc_media_t *media;

	media = libvlc_media_new_path(player->priv->vlc_inst,
				      (const char *)file);
	if (media == NULL)
		return TRUE;
	vlc_player_load_media(player, media);
	libvlc_media_release(media);

	return FALSE;
}

gboolean
gtk_vlc_player_load_uri(GtkVlcPlayer *player, const gchar *uri)
{
	libvlc_media_t *media;

	media = libvlc_media_new_location(player->priv->vlc_inst,
					  (const char *)uri);
	if (media == NULL)
		return TRUE;
	vlc_player_load_media(player, media);
	libvlc_media_release(media);

	return FALSE;
}

void
gtk_vlc_player_play(GtkVlcPlayer *player)
{
	libvlc_media_player_play(player->priv->media_player);
}

void
gtk_vlc_player_pause(GtkVlcPlayer *player)
{
	libvlc_media_player_pause(player->priv->media_player);
}

gboolean
gtk_vlc_player_toggle(GtkVlcPlayer *player)
{
	if (libvlc_media_player_is_playing(player->priv->media_player))
		gtk_vlc_player_pause(player);
	else
		gtk_vlc_player_play(player);

	return (gboolean)libvlc_media_player_is_playing(player->priv->media_player);
}

void
gtk_vlc_player_stop(GtkVlcPlayer *player)
{
	gtk_vlc_player_pause(player);
	libvlc_media_player_stop(player->priv->media_player);

	update_time(player, 0);
}

void
gtk_vlc_player_seek(GtkVlcPlayer *player, gint64 time)
{
	libvlc_media_player_set_time(player->priv->media_player,
				     (libvlc_time_t)time);
}

void
gtk_vlc_player_set_volume(GtkVlcPlayer *player, gdouble volume)
{
	libvlc_audio_set_volume(player->priv->media_player, (int)(volume*100.));
}

GtkAdjustment *
gtk_vlc_player_get_time_adjustment(GtkVlcPlayer *player)
{
	return GTK_ADJUSTMENT(player->priv->time_adjustment);
}

void
gtk_vlc_player_set_time_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj)
{
	g_signal_handler_disconnect(G_OBJECT(player->priv->time_adjustment),
				    player->priv->time_adj_on_value_changed_id);

	g_object_unref(player->priv->time_adjustment);
	player->priv->time_adjustment = GTK_OBJECT(adj);
	g_object_ref_sink(player->priv->time_adjustment);

	player->priv->time_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(player->priv->time_adjustment),
				 "value-changed",
				 G_CALLBACK(time_adj_on_value_changed), player);
}

GtkAdjustment *
gtk_vlc_player_get_volume_adjustment(GtkVlcPlayer *player)
{
	return GTK_ADJUSTMENT(player->priv->volume_adjustment);
}

void
gtk_vlc_player_set_volume_adjustment(GtkVlcPlayer *player, GtkAdjustment *adj)
{
	g_signal_handler_disconnect(G_OBJECT(player->priv->volume_adjustment),
				    player->priv->vol_adj_on_value_changed_id);

	g_object_unref(player->priv->volume_adjustment);
	player->priv->volume_adjustment = GTK_OBJECT(adj);
	g_object_ref_sink(player->priv->volume_adjustment);

	player->priv->vol_adj_on_value_changed_id =
		g_signal_connect(G_OBJECT(player->priv->volume_adjustment),
				 "value-changed",
				 G_CALLBACK(vol_adj_on_value_changed), player);
}
