#ifndef PAINT_EDITOR_H
#define PAINT_EDITOR_H

#include <gtk/gtk.h>

// Enum for tools
typedef enum {
    TOOL_PENCIL,
    TOOL_ERASER,
    TOOL_HIGHLIGHTER,
    TOOL_TEXT,
    TOOL_RECTANGLE,
    TOOL_TRIANGLE,
    TOOL_SQUARE,
    TOOL_CIRCLE,
    TOOL_LINE,
    TOOL_FILL
} Tool;

// Structure for storing points
typedef struct {
    gdouble x;
    gdouble y;
} Point;

// Structure for storing drawing actions
typedef struct {
    GArray *points;
    GdkRGBA color;
    Tool tool;
    int size;
    gdouble opacity;
    gchar *text;
} DrawingAction;

// Shared app state used by drawing functions
extern cairo_surface_t *surface;
extern GtkWidget *canvas;
extern Tool current_tool;
extern GdkRGBA current_color;
extern int brush_size;
extern GdkRGBA current_highlighter_color;
extern gdouble current_highlighter_opacity;
extern gdouble zoom_level;
extern GList *undo_stack;
extern GList *redo_stack;

void free_drawing_action(DrawingAction *action);

#endif
