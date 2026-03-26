#include "history.h"
#include "drawing.h"

void undo_action(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (undo_stack) {
        DrawingAction *action = (DrawingAction *)g_list_last(undo_stack)->data;
        undo_stack = g_list_remove(undo_stack, action);
        redo_stack = g_list_append(redo_stack, action);
        redraw_all();
    }
}

void redo_action(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (redo_stack) {
        DrawingAction *action = (DrawingAction *)g_list_last(redo_stack)->data;
        redo_stack = g_list_remove(redo_stack, action);
        undo_stack = g_list_append(undo_stack, action);
        redraw_all();
    }
}

void free_drawing_action(DrawingAction *action) {
    if (!action) return;

    if (action->points) {
        g_array_unref(action->points);
        action->points = NULL;
    }

    if (action->tool == TOOL_TEXT && action->text) {
        g_free(action->text);
        action->text = NULL;
    }

    g_free(action);
}
