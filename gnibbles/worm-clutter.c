/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>
 *              Guillaume Béland <guillaume.beland@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <config.h>
#include <glib/gprintf.h>
#include <ctype.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <libgames-support/games-runtime.h>
#include <clutter-gtk/clutter-gtk.h>
#include "main.h"
#include "gnibbles.h"
#include "level.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"

#include "worm-clutter.h"

extern GnibblesProperties *properties;
extern GdkPixbuf *worm_pixmaps[];
extern GnibblesLevel *level;
extern GnibblesBoni *boni;
extern GnibblesWarpManager *warpmanager;
extern GnibblesCWorm *cworms[NUMWORMS];


GnibblesCWorm*
gnibbles_cworm_new (guint number, guint t_xhead,
			                    guint t_yhead, gint t_direction)
{
  GnibblesCWorm *worm = g_new (GnibblesCWorm, 1);
  
  worm->actors = clutter_group_new ();
  worm->list = NULL;
  worm->number = number;
  worm->lives = SLIVES;

  worm->xhead = t_xhead;
  worm->xstart = t_xhead;
  worm->yhead = t_yhead;
  worm->ystart = t_yhead;
  worm->direction = t_direction;
  worm->direction_start = t_direction;

  gnibbles_cworm_add_actor (worm);

  return worm;
}

void
gnibbles_cworm_add_actor (GnibblesCWorm *worm)
{
  ClutterActor *actor = NULL;
  GValue val = {0,};
  gint size;

  actor = gtk_clutter_texture_new_from_pixbuf (worm_pixmaps[worm->number]);

  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean (&val, TRUE);

  clutter_actor_set_position (CLUTTER_ACTOR (actor),
                              worm->xhead * properties->tilesize,
                              worm->yhead * properties->tilesize);

  g_object_set_property (G_OBJECT (actor), "keep-aspect-ratio", &val);

  ClutterActor *tmp = NULL;

  if (worm->list) {
    gfloat w,h;
    tmp = gnibbles_cworm_get_head_actor (worm);
    clutter_actor_get_size (CLUTTER_ACTOR (tmp), &w, &h);
    size = w < h ? h : w;
    size = size / properties->tilesize;
  } else {
    size = SLENGTH; 
  }
  
  if (worm->direction == WORMRIGHT || worm->direction == WORMLEFT) {
    // if it's the worm's head, set its position    
    if (!tmp) {
      clutter_actor_set_size (CLUTTER_ACTOR (actor),
                              properties->tilesize * size,
                              properties->tilesize);

      if (worm->direction == WORMRIGHT)
        worm->xhead += size * properties->tilesize;
      else 
        worm->xhead -= size * properties->tilesize;

    } else {
      clutter_actor_set_size (CLUTTER_ACTOR (actor), 0, properties->tilesize);
    }

    g_object_set_property (G_OBJECT (actor), "repeat-x", &val);
  } else if (worm->direction == WORMDOWN || worm->direction == WORMUP) {
    // if it's the worm's head, set its position
    if (!tmp) {
      clutter_actor_set_size (CLUTTER_ACTOR (actor),
                          properties->tilesize,
                          properties->tilesize * size);

      if (worm->direction == WORMDOWN)
        worm->yhead += size * properties->tilesize;
      else 
        worm->yhead -= size * properties->tilesize;

    } else {
      clutter_actor_set_size (CLUTTER_ACTOR (actor), properties->tilesize, 0);
    }
    
    g_object_set_property (G_OBJECT (actor), "repeat-y", &val);
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), actor);  
  worm->list = g_list_prepend (worm->list, actor);
}

void
gnibbles_cworm_add_actor_with_position (GnibblesCWorm *worm, 
                                        gint t_xhead, gint t_yhead)
{
  ClutterActor *actor = NULL;
  GValue val = {0,};
  printf ("\nCALLED\n");
  actor = gtk_clutter_texture_new_from_pixbuf (worm_pixmaps[worm->number]);

  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean (&val, TRUE);

  clutter_actor_set_position (CLUTTER_ACTOR (actor),
                              t_xhead * properties->tilesize,
                              t_yhead * properties->tilesize);

  g_object_set_property (G_OBJECT (actor), "keep-aspect-ratio", &val);
  
  if (worm->direction == WORMRIGHT || worm->direction == WORMLEFT) {    
    clutter_actor_set_size (CLUTTER_ACTOR (actor), 0, properties->tilesize);
    g_object_set_property (G_OBJECT (actor), "repeat-x", &val);
  } else if (worm->direction == WORMDOWN || worm->direction == WORMUP) {
    clutter_actor_set_size (CLUTTER_ACTOR (actor), properties->tilesize, 0);
    g_object_set_property (G_OBJECT (actor), "repeat-y", &val);
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), actor);  
  worm->list = g_list_prepend (worm->list, actor);
}

void
gnibbles_cworm_destroy (GnibblesCWorm *worm)
{
  while (worm->list)
    gnibbles_cworm_remove_actor (worm);

  g_list_free (worm->list);
  g_free (worm->actors);
}

void
gnibbles_cworm_inverse (GnibblesCWorm *worm)
{
  worm->list = g_list_reverse (worm->list);
}

ClutterActor*
gnibbles_cworm_get_head_actor (GnibblesCWorm *worm)
{
  return CLUTTER_ACTOR (g_list_first (worm->list)->data);
}

ClutterActor*
gnibbles_cworm_get_tail_actor (GnibblesCWorm *worm)
{
  return CLUTTER_ACTOR (g_list_last (worm->list)->data);
}

void
gnibbles_cworm_remove_actor (GnibblesCWorm *worm)
{
  g_return_if_fail (worm->list);

  ClutterActor *tmp = gnibbles_cworm_get_tail_actor (worm);
  worm->list = g_list_delete_link (worm->list, g_list_last (worm->list));

  clutter_container_remove_actor (CLUTTER_CONTAINER (worm->actors), tmp);
}

void 
gnibbles_cworm_resize (GnibblesCWorm *worm, gint newtile)
{
  if (!worm)
    return;
  if (!worm->actors)
    return;

  int i;
  int x_pos;
  int y_pos;
  int count;    
  guint w,h;
  guint size;
  gboolean direction;
  GValue val = {0,};
  ClutterActor *tmp;

  count = clutter_group_get_n_children (CLUTTER_GROUP (worm->actors));
  load_pixmap (newtile);

  g_value_init (&val, G_TYPE_BOOLEAN);

  for (i = 0; i < count; i++) {
    tmp = clutter_group_get_nth_child (CLUTTER_GROUP (worm->actors), i);
    clutter_actor_get_position (tmp, &x_pos, &y_pos);

    clutter_actor_set_position (tmp,
                                (x_pos / properties->tilesize) * newtile,
                                (y_pos / properties->tilesize) * newtile);

    g_object_get_property (G_OBJECT (tmp), "repeat-x", &val);
    direction = g_value_get_boolean (&val);

    clutter_actor_get_size (CLUTTER_ACTOR (tmp), &w, &h);
    size = w < h ? h : w;
    size = size / properties->tilesize;

    if (direction)
      clutter_actor_set_size (tmp, newtile * size, newtile);
    else
      clutter_actor_set_size (tmp, newtile, newtile * size);

    gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (tmp), worm_pixmaps[worm->number]);
  }

}

void
gnibbles_cworm_move (ClutterTimeline *timeline, gint frame_num, gpointer data)
{
  guint w,h;
  gint x,y;
  guint size;
  gboolean direction;
  GValue val = {0,};

  GnibblesCWorm *worm = (GnibblesCWorm *)data;

  ClutterActor *first = g_list_first (worm->list)->data;
  ClutterActor *last = g_list_last (worm->list)->data;

  g_value_init (&val, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (first), "repeat-x", &val);
  direction = g_value_get_boolean (&val);

  if (first == last) {
    clutter_actor_get_position (CLUTTER_ACTOR (first), &x, &y);
    if (direction)
      clutter_actor_set_position (CLUTTER_ACTOR (first), x + properties->tilesize, y);
    else
      clutter_actor_set_position (CLUTTER_ACTOR (first), x, y + properties->tilesize);
  } else {

    clutter_actor_get_size (CLUTTER_ACTOR (first), &w, &h);
    size = w < h ? h : w;

    if (direction)
      clutter_actor_set_size (first, properties->tilesize + size, properties->tilesize);
    else
      clutter_actor_set_size (first, properties->tilesize, properties->tilesize + size);

    g_object_get_property (G_OBJECT (last), "repeat-x", &val);
    direction = g_value_get_boolean (&val);
    clutter_actor_get_size (CLUTTER_ACTOR (last), &w, &h);
    clutter_actor_get_position (CLUTTER_ACTOR (last), &x, &y);
    size = w < h ? h : w;
    size = size / (properties->tilesize + 1);

    //TODO: Set move UP/DOWn RIGHT/LEFT
    if (direction) {
      clutter_actor_set_size (last, properties->tilesize * size, properties->tilesize);
      clutter_actor_set_position (last, x + properties->tilesize, y);
      worm->xhead += properties->tilesize;
    } else {
      clutter_actor_set_size (last, properties->tilesize, properties->tilesize * size);
      clutter_actor_set_position (last, x, y + properties->tilesize);
      worm->yhead += properties->tilesize;
    }
   
    if (size <= 0)
      gnibbles_cworm_remove_actor (worm);
  }
}

void
gnibbles_cworm_move_straight_worm (GnibblesCWorm *worm)
{
  if (!(g_list_length (worm->list) == 1))
    return;

  gfloat x,y;
  ClutterActor *head = gnibbles_cworm_get_head_actor (worm);

  clutter_actor_get_position (CLUTTER_ACTOR (head), &x, &y);
  switch (worm->direction) {
    case WORMRIGHT:
      clutter_actor_set_position (CLUTTER_ACTOR (head), 
                                 x + properties->tilesize, y);
      worm->xhead++;
      break;
    case WORMDOWN:
      clutter_actor_set_position (CLUTTER_ACTOR (head), 
                                  x, y + properties->tilesize);
      worm->yhead++;
      break;
    case WORMLEFT:
      clutter_actor_set_position (CLUTTER_ACTOR (head), 
                                 x - properties->tilesize, y);
      worm->xhead--;
      break;
    case WORMUP:
      clutter_actor_set_position (CLUTTER_ACTOR (head), 
                                  x, y - properties->tilesize);
      worm->yhead--;
      break;
    default:
      break;
  }

}

void
gnibbles_cworm_move_head (GnibblesCWorm *worm)
{
  if (g_list_length (worm->list) <= 1)
    return;

  gfloat w,h;
  gfloat x,y;
  guint size;

  ClutterActor *head = gnibbles_cworm_get_head_actor (worm);

  clutter_actor_get_size (CLUTTER_ACTOR (head), &w, &h);
  clutter_actor_get_position (CLUTTER_ACTOR (head), &x, &y);
  size = w < h ? h : w;
  size = size + properties->tilesize;
  size = size / properties->tilesize;

  // set the size of the head actor 
  switch (worm->direction) {
    case WORMRIGHT:
      clutter_actor_set_size (CLUTTER_ACTOR (head), 
                              size * properties->tilesize, 
                              properties->tilesize);
      worm->xhead++;
      break;
    case WORMDOWN:
      clutter_actor_set_size (CLUTTER_ACTOR (head), 
                              properties->tilesize, 
                              size * properties->tilesize);
      worm->yhead++;
      break;
    case WORMLEFT:
      clutter_actor_set_size (CLUTTER_ACTOR (head), 
                              size * properties->tilesize, 
                              properties->tilesize);
      clutter_actor_set_position (CLUTTER_ACTOR (head), 
                                  x - properties->tilesize, y);
      worm->xhead--;
      break;
    case WORMUP:
      clutter_actor_set_size (CLUTTER_ACTOR (head), 
                              properties->tilesize, 
                              size * properties->tilesize);
      clutter_actor_set_position (CLUTTER_ACTOR (head), 
                                  x, y - properties->tilesize);
      worm->yhead--;
      break;
    default:
      break;
  }
}

void
gnibbles_cworm_move_tail (GnibblesCWorm *worm)
{
  if (g_list_length (worm->list) <= 1)
    return;

  gfloat w,h;
  gfloat x,y;
  guint size;
  gint tmp_dir;

  ClutterActor *tail = gnibbles_cworm_get_tail_actor (worm);

  clutter_actor_get_size (CLUTTER_ACTOR (tail), &w, &h);
  clutter_actor_get_position (CLUTTER_ACTOR (tail), &x, &y);
  size = w < h ? h : w;
  size = size - properties->tilesize;
  size = size / properties->tilesize;

  tmp_dir = gnibbles_cworm_get_tail_direction (worm);
  switch (tmp_dir) {
    case WORMRIGHT:
      clutter_actor_set_size (CLUTTER_ACTOR (tail), 
                              size * properties->tilesize, 
                              properties->tilesize);
      clutter_actor_set_position (CLUTTER_ACTOR (tail), 
                                x + properties->tilesize, y);
      worm->xtail++;
      break;
    case WORMDOWN:
      clutter_actor_set_size (CLUTTER_ACTOR (tail), 
                              properties->tilesize, 
                              size * properties->tilesize);
      clutter_actor_set_position (CLUTTER_ACTOR (tail), 
                                x, y + properties->tilesize);
      worm->ytail++;
      break;
    case WORMLEFT:
      clutter_actor_set_size (CLUTTER_ACTOR (tail), 
                              properties->tilesize * size, 
                              properties->tilesize);
      worm->xtail--;
      break;
    case WORMUP:
      clutter_actor_set_size (CLUTTER_ACTOR (tail), 
                              properties->tilesize, 
                              properties->tilesize * size);
      worm->ytail--;
      break;
    default:
      break;
  }

  if (size <= 0)
     gnibbles_cworm_remove_actor (worm);
}

gint
gnibbles_cworm_get_tail_direction (GnibblesCWorm *worm)
{
  gfloat w,h;
  gfloat x1,y1,x2,y2;
  gint dir = -1;
  gboolean is_horizontal;
  GValue val = {0,};
  g_value_init (&val, G_TYPE_BOOLEAN);

  ClutterActor *tail = gnibbles_cworm_get_tail_actor (worm);
  ClutterActor *next = g_list_previous (g_list_last (worm->list))->data;
  
  g_object_get_property (G_OBJECT (tail), "repeat-x", &val);
  is_horizontal = g_value_get_boolean (&val);

  clutter_actor_get_position (CLUTTER_ACTOR (next), &x2, &y2);
  clutter_actor_get_size (CLUTTER_ACTOR (next), &w, &h);
  clutter_actor_get_position (CLUTTER_ACTOR (tail), &x1, &y1);
  
  if (is_horizontal) {
    if (x2 > x1)
      dir = WORMRIGHT;
    else if (x2 == x1)
      dir = WORMLEFT;
  } else {
    if (y2 > y1)
      dir = WORMDOWN;
    else if (y2 == y1)
      dir = WORMUP;
  }

  return dir;
}

gint
gnibbles_cworm_lose_life (GnibblesCWorm * worm)
{
  worm->lives--;
  if (worm->lives < 0)
    return 1;

  return 0;
}

gint
gnibbles_cworm_can_move_to (GnibblesCWorm * worm, gint x, gint y)
{
  if (worm->xhead == x)
    return worm->yhead - 1 == y || worm->yhead + 1 == y;
  if (worm->yhead == y)
    return worm->xhead - 1 == x || worm->xhead + 1 == x;
  return FALSE;
}

void
gnibbles_cworm_position_move_head (GnibblesCWorm * worm, gint *x, gint *y)
{
  *x = worm->xhead;
  *y = worm->yhead;

  switch (worm->direction) {
    case WORMUP:
      *y = worm->yhead - 1;
      break;
    case WORMDOWN:
      *y = worm->yhead + 1;
      break;
    case WORMLEFT:
      *x = worm->xhead - 1;
      break;
    case WORMRIGHT:
      *x = worm->xhead + 1;
      break;
  }

  if (*x == BOARDWIDTH) 
    *x = 0;
  if (*x < 0) 
    *x = BOARDWIDTH - 1;
  if (*y == BOARDHEIGHT) 
    *y = 0;
  if (*y < 0) 
    *y = BOARDHEIGHT - 1;
}

gint
gnibbles_cworm_test_move_head (GnibblesCWorm * worm)
{
  int x, y;

  gnibbles_cworm_position_move_head(worm, &x, &y);

  if (level->walls[x][y] > EMPTYCHAR && level->walls[x][y] < 'z' + properties->numworms)
    return (FALSE);

  return TRUE;
}

gint
gnibbles_cworm_is_move_safe (GnibblesCWorm * worm)
{
  int x, y, i;

  gnibbles_cworm_position_move_head(worm, &x, &y);

  for (i = 0; i < properties->numworms; i++) {
    if (i != worm->number) {
      if (gnibbles_cworm_can_move_to (cworms[i], x, y))
        return (FALSE);
    }
  }

  return TRUE;
}

/* Check whether the worm will be trapped in a dead end. A location
   within the dead end and the length of the worm is given. This
   prevents worms getting trapped in a spiral, or in a corner sharper
   than 90 degrees.  runnumber is a unique number used to update the
   deadend board. The principle of the deadend board is that it marks
   all squares previously checked, so the exact size of the deadend
   can be calculated in O(n) time; to prevent the need to clear it
   afterwards, a different number is stored in the board each time
   (the number will not have been previously used, so the board will
   appear empty). Although in theory deadend_runnumber may wrap round,
   after 4 billion steps the entire board is likely to have been
   overwritten anyway. */
static guint deadendboard[BOARDWIDTH][BOARDHEIGHT] = {{0}};
static guint deadend_runnumber = 0;

static gint
gnibbles_cworm_ai_deadend (gint x, gint y, gint lengthleft)
{
  gint cdir, cx, cy;

  if (x >= BOARDWIDTH)
    x = 0;
  if (x < 0)
    x = BOARDWIDTH - 1;
  if (y >= BOARDHEIGHT)
    y = 0;
  if (y < 0)
    y = BOARDHEIGHT - 1;

  if (! lengthleft)
    return 0;

  cdir = 5;
  while (--cdir) {
    cx = x;
    cy = y;
    switch (cdir) {
    case WORMUP:
      cy -= 1;
      break;
    case WORMDOWN:
      cy += 1;
      break;
    case WORMLEFT:
      cx -= 1;
      break;
    case WORMRIGHT:
      cx += 1;
      break;
    }

    if (cx >= BOARDWIDTH)
      cx = 0;
    if (cx < 0)
      cx = BOARDWIDTH - 1;
    if (cy >= BOARDHEIGHT)
      cy = 0;
    if (cy < 0)
      cy = BOARDHEIGHT - 1;

    if ((level->walls[cx][cy] <= EMPTYCHAR
	      || level->walls[x][y] >= 'z' + properties->numworms)
	      && deadendboard[cx][cy] != deadend_runnumber) {
       
      deadendboard[cx][cy] = deadend_runnumber;
      lengthleft = gnibbles_cworm_ai_deadend(cx, cy, lengthleft - 1);
      if (!lengthleft)
	      return 0;
    }
  }
  return lengthleft;
}

/* Check a deadend starting from the next square in this direction,
   rather than from this square. Also block off the squares near worm
   heads, so that humans can't kill AI players by trapping them
   against a wall.  The given length is quartered and squared; this
   allows for the situation where the worm has gone round in a square
   and is about to get trapped in a spiral. However, it's set to at
   least BOARDWIDTH, so that on the levels with long thin paths a worm
   won't start down the path if it'll crash at the other end. */
static gint
gnibbles_cworm_ai_deadend_after (gint x, gint y, gint dir, gint length)
{
  gint cx, cy, cl, i;

  if (x < 0 || x >= BOARDWIDTH || y < 0 || y >= BOARDHEIGHT) {
    return 0;
  }

  ++deadend_runnumber;

  if (dir > 4)
    dir = 1;
  if (dir < 1)
    dir = 4;

  i = properties->numworms;
  while(i--) {
    cx = cworms[i]->xhead;
    cy = cworms[i]->yhead;
    if(cx != x || cy != y) {
      if(cx > 0) deadendboard[cx-1][cy] = deadend_runnumber;
      if(cy > 0) deadendboard[cx][cy-1] = deadend_runnumber;
      if(cx < BOARDWIDTH-1) deadendboard[cx+1][cy] = deadend_runnumber;
      if(cy < BOARDHEIGHT-1) deadendboard[cx][cy+1] = deadend_runnumber;
    }
  }

  cx = x;
  cy = y;
  switch (dir) {
  case WORMUP:
    cy -= 1;
    break;
  case WORMDOWN:
    cy += 1;
    break;
  case WORMLEFT:
    cx -= 1;
    break;
  case WORMRIGHT:
    cx += 1;
    break;
  }

  if (cx >= BOARDWIDTH)
    cx = 0;
  if (cx < 0)
    cx = BOARDWIDTH - 1;
  if (cy >= BOARDHEIGHT)
    cy = 0;
  if (cy < 0)
    cy = BOARDHEIGHT - 1;

  deadendboard[x][y] = deadend_runnumber;
  deadendboard[cx][cy] = deadend_runnumber;

  cl = (length * length) / 16;
  if (cl < BOARDWIDTH)
    cl = BOARDWIDTH;
  return gnibbles_cworm_ai_deadend (cx, cy, cl);
}

/* Check to see if another worm's head is too close in front of us;
   that is, that it's within 3 in the direction we're going and within
   1 to the side. */
static gint
gnibbles_cworm_ai_tooclose (GnibblesCWorm * worm)
{
  gint i = properties->numworms;
  gint dx, dy;
  
  while (i--) {
    dx = worm->xhead - cworms[i]->xhead;
    dy = worm->yhead - cworms[i]->yhead;
    switch (worm->direction) {
      case WORMUP:
        if (dy > 0 && dy <= 3 && dx >= -1 && dx <= 1)
 	        return 1;
        break;
      case WORMDOWN:
        if (dy < 0 && dy >= -3 && dx >= -1 && dx <= 1)
	        return 1;
        break;
      case WORMLEFT:
        if (dx > 0 && dx <= 3 && dy >= -1 && dy <= 1)
	        return 1;
        break;
      case WORMRIGHT:
        if (dx < 0 && dx >= -3 && dy >= -1 && dy <= 1)
	        return 1;
        break;
    }
  }

  return 0;
}

static gint
gnibbles_cworm_ai_wander (gint x, gint y, gint dir, gint ox, gint oy)
{
  if (dir > 4)
    dir = 1;
  if (dir < 1)
    dir = 4;

  switch (dir) {
  case WORMUP:
    y -= 1;
    break;
  case WORMDOWN:
    y += 1;
    break;
  case WORMLEFT:
    x -= 1;
    break;
  case WORMRIGHT:
    x += 1;
    break;
  }

  if (x >= BOARDWIDTH)
    x = 0;
  if (x < 0)
    x = BOARDWIDTH - 1;
  if (y >= BOARDHEIGHT)
    y = 0;
  if (y < 0)
    y = BOARDHEIGHT - 1;

  switch (level->walls[x][y] - 'A') {
    case BONUSREGULAR:
    case BONUSDOUBLE:
    case BONUSLIFE:
    case BONUSREVERSE:
      return 1;
      break;
    case BONUSHALF:
      return 0;
      break;
    default: 
      if (level->walls[x][y] > EMPTYCHAR && level->walls[x][y] < 'z' + properties->numworms) {
        return 0;
      } else {
        if (ox == x && oy == y)
	        return 0;
        return gnibbles_cworm_ai_wander (x, y, dir, ox, oy);
      }
    break;
  }
}

/* Determines the direction of the AI worm. */
void
gnibbles_cworm_ai_move (GnibblesCWorm * worm)
{
  int opposite, dir, left, right, front;
  gint bestyet, bestdir, thislen, olddir;

  opposite = (worm->direction + 1) % 4 + 1;

  front = gnibbles_cworm_ai_wander
    (worm->xhead, worm->yhead, worm->direction, worm->xhead, worm->yhead);
  left = gnibbles_cworm_ai_wander
    (worm->xhead, worm->yhead, worm->direction - 1, worm->xhead, worm->yhead);
  right = gnibbles_cworm_ai_wander
    (worm->xhead, worm->yhead, worm->direction + 1, worm->xhead, worm->yhead);

  if (!front) {
    if (left) {
      // Found a bonus to the left
      dir = worm->direction - 1;
      if (dir < 1)
        dir = 4;
      worm->direction = dir;
    } else if (right) {
      // Found a bonus to the right
      dir = worm->direction + 1;
      if (dir > 4)
        dir = 1;
      worm->direction = dir;
    } else {
      // Else move in random direction at random time intervals
      if (rand () % 30 == 1) {
        dir = worm->direction + (rand() % 2 ? 1 : -1);
        if (dir != opposite) {
          if (dir > 4)
            dir = 1;
          if (dir < 1)
            dir = 4;
          worm->direction = dir;
        }
      }
    }
  }

  /* Avoid walls, dead-ends and other worm's heads. This is done using
     an evalution function which is CAPACITY for a wall, 4 if another
     worm's head is in the tooclose area, 4 if another worm's head
     could move to the same location as ours, plus 0 if there's no
     dead-end, or the amount that doesn't fit for a deadend. olddir's
     score is reduced by 100, to favour it, but only if its score is 0
     otherwise; this is so that if we're currently trapped in a dead
     end, the worm will move in a space-filling manner in the hope
     that the dead end will disappear (e.g. if it's made from the tail
     of some worm, as often happens). */
  olddir = worm->direction;
  bestyet = CAPACITY * 2;
  bestdir = -1;

  for (dir = 1; dir <= 4; dir++) {
    worm->direction = dir;

    if (dir == opposite) 
      continue;
    thislen = 0;

    if(!gnibbles_cworm_test_move_head (worm))
      thislen += CAPACITY;

    if(gnibbles_cworm_ai_tooclose (worm))
      thislen += 4;

    if(!gnibbles_cworm_is_move_safe (worm))
      thislen += 4;

    thislen += gnibbles_cworm_ai_deadend_after
      (worm->xhead, worm->yhead, dir, worm->length + worm->change);

    if (dir == olddir && !thislen)
      thislen -= 100;
    /* If the favoured direction isn't appropriate, then choose
       another direction at random rather than favouring one in
       particular, to stop the worms bunching in the bottom-
       right corner of the board. */
    if (thislen <= 0)
      thislen -= random() % 100;
    if (thislen < bestyet) {
      bestyet = thislen;
      bestdir = dir;
    }
  }

  if (bestdir == -1) /* this should never happen, but just in case... */
    bestdir = olddir;

  worm->direction = bestdir;

  /* Make sure we are at least avoiding walls.
   * Mostly other snakes should avoid our head. */
  for (dir = 1; dir <= 4; dir++) {
    if (dir == opposite) 
      continue;
    if (!gnibbles_cworm_test_move_head (worm)) {
      worm->direction = dir;
    } else {
      continue;
    }
  }
}