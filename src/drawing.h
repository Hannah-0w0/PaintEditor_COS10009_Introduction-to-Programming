#ifndef DRAWING_H
#define DRAWING_H

#include <gtk/gtk.h>
#include "paint_editor.h"

gboolean create_surface(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
void redraw_all(void);
void draw_action(cairo_t *cr, DrawingAction *action);
void draw_brush(GtkWidget *widget, gdouble x, gdouble y);
void flood_fill(int x, int y, GdkRGBA fill_color);

#endif
