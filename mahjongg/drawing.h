/* drawing.h : Drawing routines (what else would it be)
 *
 * Copyright (C) 2003 by Callum McKenzie
 *
 * Created: <2003-09-07 10:40:24 callum>
 * Time-stamp: <2003-10-02 10:29:23 callum>
 *
 */

#ifndef DRAWING_H
#define DRAWING_H

GtkWidget * create_mahjongg_board (void);
gboolean load_images (gchar * file);
void set_background (gchar * colour);
void draw_tile (gint tileno);
void draw_all_tiles (void);

extern GdkColor bgcolour;

#endif

/* EOF */
