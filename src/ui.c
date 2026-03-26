#include "ui.h"
#include "paint_editor.h"
#include "drawing.h"
#include "history.h"
#include "events.h"

static GdkCursor *pencil_cursor = NULL;
static GdkCursor *eraser_cursor = NULL;
static GdkCursor *highlighter_cursor = NULL;
static GdkCursor *fill_cursor = NULL;
static GdkCursor *crosshair_cursor = NULL;

static GtkWidget *coord_label = NULL;
static GtkWidget *zoom_scale = NULL;
static GtkWidget *zoom_spin = NULL;

static void update_zoom_from_slider(GtkRange *range, gpointer user_data);
static void update_zoom_from_spin(GtkSpinButton *spin, gpointer user_data);

static GtkWidget *create_image_from_file(const char *filename) {
    return gtk_image_new_from_file(filename);
}

static void change_tool(GtkWidget *widget, gpointer user_data) {
    (void)widget;

    Tool new_tool = GPOINTER_TO_INT(user_data);
    g_print("Changing tool to: %d\n", new_tool);

    current_tool = new_tool;

    if (canvas) {
        GdkWindow *window = gtk_widget_get_window(canvas);
        if (window) {
            switch (current_tool) {
                case TOOL_PENCIL:
                    gdk_window_set_cursor(window, pencil_cursor);
                    break;
                case TOOL_ERASER:
                    gdk_window_set_cursor(window, eraser_cursor);
                    break;
                case TOOL_HIGHLIGHTER:
                    gdk_window_set_cursor(window, highlighter_cursor);
                    break;
                case TOOL_FILL:
                    gdk_window_set_cursor(window, fill_cursor);
                    break;
                case TOOL_RECTANGLE:
                case TOOL_SQUARE:
                case TOOL_TRIANGLE:
                case TOOL_CIRCLE:
                case TOOL_LINE:
                    gdk_window_set_cursor(window, crosshair_cursor);
                    break;
                case TOOL_TEXT:
                    gdk_window_set_cursor(window, gdk_cursor_new_from_name(gdk_display_get_default(), "text"));
                    break;
                default:
                    gdk_window_set_cursor(window, NULL);
                    break;
            }
        }
    }
}

static void change_color(GtkColorButton *color_button, gpointer user_data) {
    (void)user_data;

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_button), &current_color);
    g_print("Color changed to: RGBA(%f, %f, %f, %f)\n",
            current_color.red,
            current_color.green,
            current_color.blue,
            current_color.alpha);
}

static void change_highlighter_color(GtkColorButton *color_button, gpointer user_data) {
    (void)user_data;

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_button), &current_highlighter_color);
    g_print("Highlighter color changed to: RGBA(%f, %f, %f, %f)\n",
            current_highlighter_color.red,
            current_highlighter_color.green,
            current_highlighter_color.blue,
            current_highlighter_color.alpha);
}

static void update_tool_size(GtkRange *range, gpointer user_data) {
    (void)user_data;

    brush_size = gtk_range_get_value(range);
    g_print("Tool size changed to: %d\n", brush_size);
}

static void update_highlighter_opacity(GtkRange *range, gpointer user_data) {
    (void)user_data;

    current_highlighter_opacity = gtk_range_get_value(range) / 100.0;
    g_print("Highlighter opacity changed to: %f\n", current_highlighter_opacity);
}

void update_zoom_level(gdouble new_zoom) {
    zoom_level = new_zoom / 100.0;

    g_signal_handlers_block_by_func(zoom_scale, update_zoom_from_slider, NULL);
    g_signal_handlers_block_by_func(zoom_spin, update_zoom_from_spin, NULL);

    gtk_range_set_value(GTK_RANGE(zoom_scale), new_zoom);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_spin), new_zoom);

    g_signal_handlers_unblock_by_func(zoom_scale, update_zoom_from_slider, NULL);
    g_signal_handlers_unblock_by_func(zoom_spin, update_zoom_from_spin, NULL);

    if (canvas) {
        gtk_widget_queue_draw(canvas);
    }
}

static void update_zoom_from_slider(GtkRange *range, gpointer user_data) {
    (void)user_data;

    gdouble new_zoom = gtk_range_get_value(range);
    update_zoom_level(new_zoom);
}

static void update_zoom_from_spin(GtkSpinButton *spin, gpointer user_data) {
    (void)user_data;

    gdouble new_zoom = gtk_spin_button_get_value(spin);
    update_zoom_level(new_zoom);
}

void create_tool_cursors(void) {
    GdkDisplay *display = gdk_display_get_default();
    GdkPixbuf *pixbuf;

    pixbuf = gdk_pixbuf_new_from_file("assets/icons/pencil.png", NULL);
    if (pixbuf) {
        pencil_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    pixbuf = gdk_pixbuf_new_from_file("assets/icons/eraser.png", NULL);
    if (pixbuf) {
        eraser_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    pixbuf = gdk_pixbuf_new_from_file("assets/icons/fill_bucket.png", NULL);
    if (pixbuf) {
        fill_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    pixbuf = gdk_pixbuf_new_from_file("assets/icons/highlighter.png", NULL);
    if (pixbuf) {
        highlighter_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    crosshair_cursor = gdk_cursor_new_from_name(display, "crosshair");
}

void destroy_tool_cursors(void) {
    if (pencil_cursor) g_object_unref(pencil_cursor);
    if (eraser_cursor) g_object_unref(eraser_cursor);
    if (highlighter_cursor) g_object_unref(highlighter_cursor);
    if (fill_cursor) g_object_unref(fill_cursor);
    if (crosshair_cursor) g_object_unref(crosshair_cursor);
}

GtkWidget *create_toolbar(void) {
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);

    GtkToolItem *undo_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("edit-undo", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Undo"
    );
    gtk_tool_item_set_tooltip_text(undo_button, "Undo");
    g_signal_connect(undo_button, "clicked", G_CALLBACK(undo_action), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), undo_button, -1);

    GtkToolItem *redo_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("edit-redo", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Redo"
    );
    gtk_tool_item_set_tooltip_text(redo_button, "Redo");
    g_signal_connect(redo_button, "clicked", G_CALLBACK(redo_action), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), redo_button, -1);

    GtkToolItem *sep = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep, -1);

    GtkToolItem *save_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("document-save", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Save"
    );
    gtk_tool_item_set_tooltip_text(save_button, "Save");
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save_button, -1);

    GtkToolItem *clear_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("edit-clear", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Clear"
    );
    gtk_tool_item_set_tooltip_text(clear_button, "Clear");
    g_signal_connect(clear_button, "clicked", G_CALLBACK(on_clear_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), clear_button, -1);

    return toolbar;
}

GtkWidget *create_tools_toolbar(void) {
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);

    const struct {
        const char *icon_file;
        const char *tooltip;
        Tool tool;
    } tools[] = {
        {"assets/icons/pencil.png", "Pencil", TOOL_PENCIL},
        {"assets/icons/eraser.png", "Eraser", TOOL_ERASER},
        {"assets/icons/highlighter.png", "Highlighter", TOOL_HIGHLIGHTER},
        {"assets/icons/textbox.png", "Text", TOOL_TEXT},
        {"assets/icons/rectangle.png", "Rectangle", TOOL_RECTANGLE},
        {"assets/icons/triangle.png", "Triangle", TOOL_TRIANGLE},
        {"assets/icons/square.png", "Square", TOOL_SQUARE},
        {"assets/icons/circle.png", "Circle", TOOL_CIRCLE},
        {"assets/icons/line.png", "Line", TOOL_LINE},
        {"assets/icons/fill_bucket.png", "Fill", TOOL_FILL},
        {NULL, NULL, 0}
    };

    for (int i = 0; tools[i].icon_file != NULL; i++) {
        GtkToolItem *tool_item = gtk_tool_button_new(
            create_image_from_file(tools[i].icon_file),
            tools[i].tooltip
        );
        gtk_tool_item_set_tooltip_text(tool_item, tools[i].tooltip);
        g_signal_connect(tool_item, "clicked", G_CALLBACK(change_tool), GINT_TO_POINTER(tools[i].tool));
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
    }

    GtkToolItem *sep = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep, -1);

    GtkToolItem *color_item = gtk_tool_item_new();
    GtkWidget *color_button = gtk_color_button_new();
    gtk_container_add(GTK_CONTAINER(color_item), color_button);
    g_signal_connect(color_button, "color-set", G_CALLBACK(change_color), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), color_item, -1);

    GtkToolItem *highlighter_color_item = gtk_tool_item_new();
    GtkWidget *highlighter_button = gtk_color_button_new_with_rgba(&current_highlighter_color);
    gtk_container_add(GTK_CONTAINER(highlighter_color_item), highlighter_button);
    g_signal_connect(highlighter_button, "color-set", G_CALLBACK(change_highlighter_color), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), highlighter_color_item, -1);

    return toolbar;
}

GtkWidget *create_canvas(void) {
    GtkWidget *fixed = gtk_fixed_new();
    GtkWidget *drawing_area = gtk_drawing_area_new();

    gtk_widget_set_size_request(drawing_area, 1850, 1000);
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);

    gtk_widget_set_halign(drawing_area, GTK_ALIGN_FILL);
    gtk_widget_set_valign(drawing_area, GTK_ALIGN_FILL);

    gtk_widget_add_events(drawing_area,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_SCROLL_MASK);

    g_signal_connect(drawing_area, "configure-event", G_CALLBACK(create_surface), NULL);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_event), NULL);
    g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_button_press_event), NULL);
    g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify_event), NULL);
    g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(on_button_release_event), NULL);
    g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(on_scroll_event), NULL);

    gtk_widget_set_can_focus(drawing_area, TRUE);

    gtk_fixed_put(GTK_FIXED(fixed), drawing_area, 0, 0);
    canvas = drawing_area;
    return fixed;
}

void create_bottom_toolbar(GtkWidget *vbox) {
    GtkWidget *bottom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(bottom_box), 5);

    GtkWidget *coord_frame = gtk_frame_new(NULL);
    coord_label = gtk_label_new("X: 0, Y: 0");
    gtk_container_add(GTK_CONTAINER(coord_frame), coord_label);
    gtk_box_pack_start(GTK_BOX(bottom_box), coord_frame, FALSE, FALSE, 5);

    GtkWidget *zoom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(bottom_box), zoom_box, FALSE, FALSE, 0);

    GtkWidget *zoom_label = gtk_label_new("Zoom:");
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_label, FALSE, FALSE, 5);

    zoom_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 1000, 10);
    gtk_scale_set_draw_value(GTK_SCALE(zoom_scale), FALSE);
    gtk_range_set_value(GTK_RANGE(zoom_scale), 100);
    gtk_widget_set_size_request(zoom_scale, 150, -1);
    g_signal_connect(zoom_scale, "value-changed", G_CALLBACK(update_zoom_from_slider), NULL);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_scale, FALSE, FALSE, 0);

    zoom_spin = gtk_spin_button_new_with_range(10, 1000, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_spin), 100);
    gtk_entry_set_width_chars(GTK_ENTRY(zoom_spin), 5);
    g_signal_connect(zoom_spin, "value-changed", G_CALLBACK(update_zoom_from_spin), NULL);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_spin, FALSE, FALSE, 5);

    GtkWidget *percent_label = gtk_label_new("%");
    gtk_box_pack_start(GTK_BOX(zoom_box), percent_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), bottom_box, FALSE, FALSE, 0);
}

GtkWidget *create_size_slider(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    GtkWidget *label = gtk_label_new("Size");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), brush_size);
    gtk_widget_set_size_request(scale, 30, 200);

    g_signal_connect(scale, "value-changed", G_CALLBACK(update_tool_size), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);

    return vbox;
}

GtkWidget *create_opacity_slider(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    GtkWidget *label = gtk_label_new("Opacity");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), current_highlighter_opacity * 100);
    gtk_widget_set_size_request(scale, 30, 200);

    g_signal_connect(scale, "value-changed", G_CALLBACK(update_highlighter_opacity), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);

    return vbox;
}

void ui_update_coordinates(gint x, gint y) {
    if (!coord_label) return;

    char coord_text[32];
    g_snprintf(coord_text, sizeof(coord_text), "X: %d, Y: %d", x, y);
    gtk_label_set_text(GTK_LABEL(coord_label), coord_text);
}
