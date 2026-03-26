#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

void create_tool_cursors(void);
void destroy_tool_cursors(void);
GtkWidget *create_toolbar(void);
GtkWidget *create_tools_toolbar(void);
GtkWidget *create_canvas(void);
void create_bottom_toolbar(GtkWidget *vbox);
GtkWidget *create_size_slider(void);
GtkWidget *create_opacity_slider(void);
void update_zoom_level(gdouble new_zoom);
void ui_update_coordinates(gint x, gint y);

#endif
