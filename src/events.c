#include <math.h>
#include "events.h"
#include "paint_editor.h"
#include "drawing.h"
#include "audio.h"
#include "history.h"
#include "ui.h"

static Point shape_start;
static gboolean is_drawing_shape = FALSE;
static DrawingAction *current_action = NULL;

static void on_text_entered(GtkEntry *entry, gpointer user_data) {
    (void)user_data;

    const gchar *text = gtk_entry_get_text(entry);
    if (text && *text) {
        cairo_t *cr = cairo_create(surface);

        cairo_set_source_rgba(cr,
            current_color.red,
            current_color.green,
            current_color.blue,
            current_color.alpha);
        cairo_set_font_size(cr, brush_size);

        GtkAllocation allocation;
        gtk_widget_get_allocation(GTK_WIDGET(entry), &allocation);
        cairo_move_to(cr, allocation.x / zoom_level, allocation.y / zoom_level + brush_size);
        cairo_show_text(cr, text);

        cairo_destroy(cr);
        gtk_widget_queue_draw(canvas);

        DrawingAction *text_action = g_new(DrawingAction, 1);
        text_action->points = g_array_new(FALSE, FALSE, sizeof(Point));
        Point p = {allocation.x / zoom_level, allocation.y / zoom_level};
        g_array_append_val(text_action->points, p);
        text_action->color = current_color;
        text_action->tool = TOOL_TEXT;
        text_action->size = brush_size;
        text_action->text = g_strdup(text);

        g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
        redo_stack = NULL;
        undo_stack = g_list_append(undo_stack, text_action);
    }

    gtk_widget_destroy(GTK_WIDGET(entry));
}

void on_clear_clicked(GtkWidget *widget, gpointer user_data) {
    (void)user_data;

    GtkWidget *confirm_dialog = gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(widget)),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Are you sure you want to clear the canvas?\nThis action cannot be undone."
    );

    gint response = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
    gtk_widget_destroy(confirm_dialog);

    if (response == GTK_RESPONSE_YES) {
        if (surface) {
            cairo_t *cr = cairo_create(surface);
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_paint(cr);
            cairo_destroy(cr);

            g_list_free_full(undo_stack, (GDestroyNotify)free_drawing_action);
            g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
            undo_stack = NULL;
            redo_stack = NULL;

            if (current_action) {
                free_drawing_action(current_action);
                current_action = NULL;
            }

            is_drawing_shape = FALSE;
            gtk_widget_queue_draw(canvas);
        }
    }
}

void on_save_clicked(GtkWidget *widget, gpointer user_data) {
    (void)user_data;

    GtkWidget *dialog;
    GtkFileChooser *chooser;

    dialog = gtk_file_chooser_dialog_new("Save File",
                                        GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Save", GTK_RESPONSE_ACCEPT,
                                        NULL);
    chooser = GTK_FILE_CHOOSER(dialog);

    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    gtk_file_chooser_set_current_name(chooser, "Untitled.png");

    GtkFileFilter *filter_png = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_png, "PNG files");
    gtk_file_filter_add_pattern(filter_png, "*.png");
    gtk_file_chooser_add_filter(chooser, filter_png);

    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All files");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(chooser, filter_all);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(chooser);

        if (surface) {
            cairo_surface_t *save_surface = cairo_image_surface_create(
                CAIRO_FORMAT_ARGB32,
                cairo_image_surface_get_width(surface),
                cairo_image_surface_get_height(surface)
            );

            cairo_t *cr = cairo_create(save_surface);
            cairo_set_source_surface(cr, surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);

            cairo_status_t status = cairo_surface_write_to_png(save_surface, filename);
            if (status != CAIRO_STATUS_SUCCESS) {
                GtkWidget *error_dialog = gtk_message_dialog_new(
                    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Error saving file: %s",
                    cairo_status_to_string(status)
                );
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
            }

            cairo_surface_destroy(save_surface);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (!surface) return FALSE;

    cairo_scale(cr, zoom_level, zoom_level);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    return FALSE;
}

gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)user_data;

    if (event->button == GDK_BUTTON_PRIMARY) {
        gdouble x = event->x / zoom_level;
        gdouble y = event->y / zoom_level;

        g_print("Button press: tool=%d, x=%f, y=%f\n", current_tool, x, y);

        switch (current_tool) {
            case TOOL_FILL:
                flood_fill(event->x, event->y, current_color);
                play_tool_sound(TOOL_FILL);
                break;

            case TOOL_TEXT: {
                GtkWidget *entry = gtk_entry_new();
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type text...");
                gtk_widget_set_size_request(entry, 100, -1);
                gtk_fixed_put(GTK_FIXED(gtk_widget_get_parent(widget)), entry, x, y);
                gtk_widget_show(entry);
                gtk_widget_grab_focus(entry);
                g_signal_connect(entry, "activate", G_CALLBACK(on_text_entered), NULL);
                break;
            }

            case TOOL_RECTANGLE:
            case TOOL_SQUARE:
            case TOOL_CIRCLE:
            case TOOL_TRIANGLE:
            case TOOL_LINE:
                shape_start.x = x;
                shape_start.y = y;
                is_drawing_shape = TRUE;
                break;

            default: {
                current_action = g_new(DrawingAction, 1);
                current_action->points = g_array_new(FALSE, FALSE, sizeof(Point));
                current_action->color = (current_tool == TOOL_HIGHLIGHTER) ?
                                      current_highlighter_color : current_color;
                current_action->tool = current_tool;
                current_action->size = brush_size;
                current_action->opacity = (current_tool == TOOL_HIGHLIGHTER) ?
                                        current_highlighter_opacity : 1.0;
                current_action->text = NULL;

                Point p = {x, y};
                g_array_append_val(current_action->points, p);
                draw_brush(widget, x, y);

                if (current_tool == TOOL_PENCIL ||
                    current_tool == TOOL_ERASER ||
                    current_tool == TOOL_HIGHLIGHTER) {
                    play_tool_sound(current_tool);
                }
                break;
            }
        }
    }
    return TRUE;
}

gboolean on_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;

    gdouble x = event->x / zoom_level;
    gdouble y = event->y / zoom_level;

    ui_update_coordinates((int)event->x, (int)event->y);

    if (is_drawing_shape) {
        redraw_all();
        cairo_t *cr = cairo_create(surface);

        cairo_set_source_rgba(cr,
            current_color.red,
            current_color.green,
            current_color.blue,
            current_color.alpha);
        cairo_set_line_width(cr, brush_size);

        switch (current_tool) {
            case TOOL_RECTANGLE:
                cairo_rectangle(cr, shape_start.x, shape_start.y,
                              x - shape_start.x, y - shape_start.y);
                break;

            case TOOL_SQUARE: {
                double size = fmin(fabs(x - shape_start.x), fabs(y - shape_start.y));
                cairo_rectangle(cr, shape_start.x, shape_start.y, size, size);
                break;
            }

            case TOOL_CIRCLE: {
                double radius = hypot(x - shape_start.x, y - shape_start.y);
                cairo_arc(cr, shape_start.x, shape_start.y, radius, 0, 2 * G_PI);
                break;
            }

            case TOOL_TRIANGLE:
                cairo_move_to(cr, shape_start.x, shape_start.y);
                cairo_line_to(cr, x, y);
                cairo_line_to(cr, shape_start.x - (x - shape_start.x), y);
                cairo_close_path(cr);
                break;

            case TOOL_LINE:
                cairo_move_to(cr, shape_start.x, shape_start.y);
                cairo_line_to(cr, x, y);
                break;

            default:
                break;
        }

        cairo_stroke(cr);
        cairo_destroy(cr);
        gtk_widget_queue_draw(widget);
    } else if (current_action) {
        Point p = {x, y};
        g_array_append_val(current_action->points, p);
        draw_brush(widget, x, y);

        if (current_tool == TOOL_PENCIL || current_tool == TOOL_ERASER || current_tool == TOOL_HIGHLIGHTER) {
            play_tool_sound(current_tool);
        }
    }

    return TRUE;
}

gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (event->button == GDK_BUTTON_PRIMARY) {
        gdouble x = event->x / zoom_level;
        gdouble y = event->y / zoom_level;

        if (is_drawing_shape) {
            is_drawing_shape = FALSE;
            DrawingAction *shape_action = g_new(DrawingAction, 1);
            shape_action->points = g_array_new(FALSE, FALSE, sizeof(Point));
            shape_action->color = current_color;
            shape_action->tool = current_tool;
            shape_action->size = brush_size;
            shape_action->text = NULL;

            g_array_append_val(shape_action->points, shape_start);
            Point end_point = {x, y};
            g_array_append_val(shape_action->points, end_point);

            g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
            redo_stack = NULL;
            undo_stack = g_list_append(undo_stack, shape_action);

            redraw_all();
        } else if (current_action) {
            g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
            redo_stack = NULL;
            undo_stack = g_list_append(undo_stack, current_action);
            current_action = NULL;
        }

        stop_tool_sound();
    }
    return TRUE;
}

gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (event->state & GDK_CONTROL_MASK) {
        gdouble new_zoom = zoom_level * 100;

        if (event->direction == GDK_SCROLL_UP) {
            new_zoom += 10;
        } else if (event->direction == GDK_SCROLL_DOWN) {
            new_zoom -= 10;
        }

        new_zoom = CLAMP(new_zoom, 10, 1000);
        update_zoom_level(new_zoom);

        return TRUE;
    }
    return FALSE;
}

void events_cleanup_state(void) {
    if (current_action) {
        free_drawing_action(current_action);
        current_action = NULL;
    }
    is_drawing_shape = FALSE;
}
