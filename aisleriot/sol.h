/* Aisleriot - sol.h
 * Copyright (C) 1998, 2003 Jonathan Blandford <jrb@mit.edu>
 *
 * This game is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef SOL_H
#define SOL_H
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include "press_data.h"

/*
 * Constants
 */
#define GAMESDIR "sol-games/"
#define GAME_EVENTS (GDK_EXPOSURE_MASK        |\
		     GDK_BUTTON_PRESS_MASK    |\
		     GDK_POINTER_MOTION_MASK  |\
		     GDK_BUTTON_RELEASE_MASK)

/* GConf keys. */
#define WIDTH_GCONF_KEY "/apps/aisleriot/width"
#define HEIGHT_GCONF_KEY "/apps/aisleriot/height"
#define THEME_GCONF_KEY "/apps/aisleriot/card_style"
#define RECENT_GAMES_GCONF_KEY "/apps/aisleriot/recent_games"

/* Feature masks */
#define DROPPABLE_FMASK 1
#define SCOREHIDDEN_FMASK 2

/*
 * Global variables
 */

extern GtkWidget*       app;
extern GtkWidget*       playing_area;
extern GtkWidget*       statusbar;
extern GdkGC*           draw_gc;
extern GdkGC*           slot_gc;
extern GdkGC*           bg_gc;
extern GdkPixmap*       surface;
extern gchar*	  	card_style;

extern gboolean		droppable_is_featured;
extern gboolean		score_is_hidden;

extern guint            timeout;
extern guint            seed;
extern gchar*           game_file;
extern gchar*           game_name;
extern gboolean         game_in_progress;
extern gboolean         game_over;
extern gboolean         game_won;
extern gboolean         click_to_move;
extern gchar            *gamesdir;

extern GConfClient * gconf_client;

gchar* game_file_to_name(const gchar* file);
void new_game(gchar* file, guint *seed);
void quit_app(GtkMenuItem*);
void set_score(guint new_score);
guint get_score(void);
void timer_start(void);
void timer_restart(void);
void timer_stop(void);
void timer_reset(void);
guint timer_get(void);
void eval_installed_file(gchar *file);
void add_recently_played_game (gchar* game_file);

#endif
