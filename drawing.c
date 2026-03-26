#include "drawing.h"
#include <math.h>

// Create surface for drawing
gboolean create_surface(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
    (void)event;
    (void)data;

    g_print("Creating surface...\n");  // Debug print

    if (surface) {
        cairo_surface_destroy(surface);
    }

    GdkWindow *window = gtk_widget_get_window(widget);
    if (!window) {
        g_print("No window available!\n");
        return FALSE;
    }

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    g_print("Creating surface with dimensions: %d x %d\n", width, height);  // Debug print

    surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32,
        width,
        height
    );

    // Initialize surface with white background
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);

    g_print("Surface created successfully\n");  // Debug print
    return TRUE;
}

// Redraw the entire canvas
void redraw_all(void) {
    if (!surface) return;

    // Clear surface
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // First draw all non-fill actions
    for (GList *l = undo_stack; l != NULL; l = l->next) {
        DrawingAction *action = (DrawingAction *)l->data;
        if (action->tool != TOOL_FILL) {
            draw_action(cr, action);
        }
    }

    // Then draw all fill actions
    for (GList *l = undo_stack; l != NULL; l = l->next) {
        DrawingAction *action = (DrawingAction *)l->data;
        if (action->tool == TOOL_FILL) {
            draw_action(cr, action);
        }
    }

    cairo_destroy(cr);
    gtk_widget_queue_draw(canvas);
}

// Draw a single action on the surface
void draw_action(cairo_t *cr, DrawingAction *action) {
    if (!action || !action->points || action->points->len == 0)
        return;

    Point *points = (Point *)action->points->data;

    // Save the current drawing state
    cairo_save(cr);

    // Apply zoom transformation for consistent rendering at different zoom levels
    cairo_scale(cr, 1.0 / zoom_level, 1.0 / zoom_level);

    // Set color based on tool type
    if (action->tool == TOOL_ERASER) {
        // Force white color for eraser
        cairo_set_source_rgb(cr, 1, 1, 1);
    } else if (action->tool == TOOL_HIGHLIGHTER) {
        cairo_set_source_rgba(cr,
            action->color.red,
            action->color.green,
            action->color.blue,
            action->opacity);
    } else {
        cairo_set_source_rgba(cr,
            action->color.red,
            action->color.green,
            action->color.blue,
            action->color.alpha);
    }

    // Line properties
    cairo_set_line_width(cr, action->size);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); // Round end caps
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); // Round curves

    // Draw based on tool type
    switch (action->tool) {
        case TOOL_PENCIL:
        case TOOL_ERASER:
        case TOOL_HIGHLIGHTER:
            // Draw freehand lines (pencil strokes, eraser, highlighter)
            cairo_move_to(cr, points[0].x, points[0].y);
            for (int i = 1; i < action->points->len; i++) {
                cairo_line_to(cr, points[i].x, points[i].y);
            }
            cairo_stroke(cr);
            break;

        case TOOL_RECTANGLE:
            // Draw rectangle from start to end points
            cairo_rectangle(cr, points[0].x, points[0].y,
                          points[1].x - points[0].x,
                          points[1].y - points[0].y);
            cairo_stroke(cr);
            break;

        case TOOL_SQUARE: {
            double size = fmin(fabs(points[1].x - points[0].x),
                             fabs(points[1].y - points[0].y));
            cairo_rectangle(cr, points[0].x, points[0].y, size, size);
            cairo_stroke(cr);
            break;
        }

        case TOOL_CIRCLE: {
            double radius = hypot(points[1].x - points[0].x,
                                points[1].y - points[0].y);
            cairo_arc(cr, points[0].x, points[0].y, radius, 0, 2 * G_PI);
            cairo_stroke(cr);
            break;
        }

        case TOOL_TRIANGLE:
            cairo_move_to(cr, points[0].x, points[0].y);
            cairo_line_to(cr, points[1].x, points[1].y);
            cairo_line_to(cr, points[0].x - (points[1].x - points[0].x),
                         points[1].y);
            cairo_close_path(cr);
            cairo_stroke(cr);
            break;

        case TOOL_LINE:
            cairo_move_to(cr, points[0].x, points[0].y);
            cairo_line_to(cr, points[1].x, points[1].y);
            cairo_stroke(cr);
            break;

        case TOOL_FILL:
            // Redraw fill action
            flood_fill((int)(points[0].x * zoom_level),
                      (int)(points[0].y * zoom_level),
                      action->color);
            break;

        case TOOL_TEXT:
            if (action->text) {
                cairo_set_font_size(cr, action->size);
                cairo_move_to(cr, points[0].x * zoom_level,
                                 points[0].y * zoom_level + action->size);
                cairo_show_text(cr, action->text);
            }
            break;

        default:
            break;
    }
    cairo_restore(cr);
}

static void enqueue_fill_neighbor(GQueue *queue, int x, int y) {
    Point *p = g_new(Point, 1);
    p->x = x;
    p->y = y;
    g_queue_push_tail(queue, p);
}

// Draw an immediate brush stroke
void draw_brush(GtkWidget *widget, gdouble x, gdouble y) {
    cairo_t *cr = cairo_create(surface);

    // Apply zoom transformation
    cairo_scale(cr, 1.0 / zoom_level, 1.0 / zoom_level);

    // Set color and properties based on current tool
    if (current_tool == TOOL_ERASER) {
        cairo_set_source_rgb(cr, 1, 1, 1);
    } else if (current_tool == TOOL_HIGHLIGHTER) {
        cairo_set_source_rgba(cr,
            current_highlighter_color.red,
            current_highlighter_color.green,
            current_highlighter_color.blue,
            current_highlighter_opacity);
    } else {
        cairo_set_source_rgba(cr,
            current_color.red,
            current_color.green,
            current_color.blue,
            current_color.alpha);
    }

    // Scale the brush size with zoom
    cairo_set_line_width(cr, brush_size);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    // Draw at zoomed coordinates
    cairo_move_to(cr, x * zoom_level, y * zoom_level);
    cairo_line_to(cr, x * zoom_level, y * zoom_level);
    cairo_stroke(cr);

    cairo_destroy(cr);
    gtk_widget_queue_draw(widget);
}

// Handle fill tool click
void flood_fill(int x, int y, GdkRGBA fill_color) {
    g_print("flood_fill called: x=%d, y=%d\n", x, y);  // Debug print

    if (!surface) {
        g_print("No surface to fill\n");
        return;
    }

    // Convert coordinates based on zoom level
    x = (int)(x / zoom_level);
    y = (int)(y / zoom_level);
    g_print("Converted coordinates: x=%d, y=%d\n", x, y);  // Debug print

    // Get surface data
    cairo_surface_flush(surface);
    unsigned char *data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    g_print("Surface dimensions: width=%d, height=%d, stride=%d\n", width, height, stride);  // Debug print

    // Check bounds
    if (x < 0 || x >= width || y < 0 || y >= height) {
        g_print("Coordinates out of bounds!\n");  // Debug print
        return;
    }

    // Get target color (color being replaced)
    int pos = y * stride + x * 4;
    unsigned char target_r = data[pos + 2];
    unsigned char target_g = data[pos + 1];
    unsigned char target_b = data[pos + 0];
    unsigned char target_a = data[pos + 3];

    // Convert fill color to unsigned char format
    unsigned char fill_r = (unsigned char)(fill_color.red * 255);
    unsigned char fill_g = (unsigned char)(fill_color.green * 255);
    unsigned char fill_b = (unsigned char)(fill_color.blue * 255);
    unsigned char fill_a = (unsigned char)(fill_color.alpha * 255);

    // Don't fill if colors are the same
    if (target_r == fill_r && target_g == fill_g &&
        target_b == fill_b && target_a == fill_a) {
        return;
    }

    // Create queue and visited array (Queue-based flood fill algorithm)
    GQueue *queue = g_queue_new();
    gboolean *visited = g_new0(gboolean, width * height);

    // Add start point
    enqueue_fill_neighbor(queue, x, y);

    while (!g_queue_is_empty(queue)) {
        Point *p = g_queue_pop_head(queue);
        int px = (int)p->x;
        int py = (int)p->y;
        g_free(p);

        if (px < 0 || px >= width || py < 0 || py >= height)
            continue;

        pos = py * stride + px * 4;
        int idx = py * width + px;

        if (visited[idx] ||
            data[pos + 2] != target_r ||
            data[pos + 1] != target_g ||
            data[pos + 0] != target_b ||
            data[pos + 3] != target_a)
            continue;

        // Fill pixel
        data[pos + 2] = fill_r;
        data[pos + 1] = fill_g;
        data[pos + 0] = fill_b;
        data[pos + 3] = fill_a;
        visited[idx] = TRUE; // mark as visited

        // Add neighboring pixels
        enqueue_fill_neighbor(queue, px + 1, py);
        enqueue_fill_neighbor(queue, px - 1, py);
        enqueue_fill_neighbor(queue, px, py + 1);
        enqueue_fill_neighbor(queue, px, py - 1);
    }

    g_queue_free(queue);
    g_free(visited);

    cairo_surface_mark_dirty(surface);
    gtk_widget_queue_draw(canvas);

    // Create fill action for undo stack
    DrawingAction *fill_action = g_new(DrawingAction, 1);
    fill_action->points = g_array_new(FALSE, FALSE, sizeof(Point));
    Point p = {x / zoom_level, y / zoom_level};  // Store unzoomed coordinates
    g_array_append_val(fill_action->points, p);
    fill_action->color = fill_color;
    fill_action->tool = TOOL_FILL;
    fill_action->size = 1;
    fill_action->text = NULL;

    // Clear redo stack and add to undo stack
    g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
    redo_stack = NULL;
    undo_stack = g_list_append(undo_stack, fill_action);
}
