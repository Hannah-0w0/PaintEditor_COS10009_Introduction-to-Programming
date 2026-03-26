#ifndef HISTORY_H
#define HISTORY_H

#include <gtk/gtk.h>
#include "paint_editor.h"

void undo_action(GtkWidget *widget, gpointer user_data);
void redo_action(GtkWidget *widget, gpointer user_data);
void free_drawing_action(DrawingAction *action);

#endif
