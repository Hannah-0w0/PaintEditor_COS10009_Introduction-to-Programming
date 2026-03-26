#include <gtk/gtk.h>
#include "paint_editor.h"
#include "audio.h"
#include "history.h"
#include "ui.h"
#include "events.h"

/*
 * Module map:
 * - paint_editor.c: app bootstrap, shared state, lifecycle cleanup.
 * - ui.c: toolbars, sliders, canvas widget wiring, cursor and zoom UI.
 * - events.c: mouse/scroll/text event handling and action creation.
 * - drawing.c: rendering, redraw pipeline, brush strokes, flood fill.
 * - history.c: undo/redo stacks and DrawingAction memory cleanup.
 * - audio.c: tool sound initialization, playback, and shutdown.
 */

// Shared drawing state
cairo_surface_t *surface = NULL;
GtkWidget *canvas = NULL;
Tool current_tool = TOOL_PENCIL;
GdkRGBA current_color = {0, 0, 0, 1};
int brush_size = 10;
GdkRGBA current_highlighter_color = {1, 1, 0, 0.5};
gdouble current_highlighter_opacity = 0.5;
gdouble zoom_level = 1.0;
GList *undo_stack = NULL;
GList *redo_stack = NULL;

static void cleanup_resources(void) {
    if (undo_stack) {
        g_list_free_full(undo_stack, (GDestroyNotify)free_drawing_action);
        undo_stack = NULL;
    }

    if (redo_stack) {
        g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
        redo_stack = NULL;
    }

    events_cleanup_state();

    if (surface) {
        cairo_surface_destroy(surface);
        surface = NULL;
    }

    destroy_tool_cursors();
    cleanup_audio();
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    GtkWidget *window;
    GtkWidget *main_vbox;
    GtkWidget *content_hbox;
    GtkWidget *tools_vbox;
    GtkWidget *toolbar;
    GtkWidget *tools_toolbar;

    init_audio();

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Paint Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    create_tool_cursors();

    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    toolbar = create_toolbar();
    gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *tools_toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    tools_toolbar = create_tools_toolbar();
    gtk_box_pack_start(GTK_BOX(tools_toolbar_box), tools_toolbar, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), tools_toolbar_box, FALSE, FALSE, 0);

    content_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), content_hbox, TRUE, TRUE, 0);

    tools_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(tools_vbox, 40, -1);
    gtk_box_pack_start(GTK_BOX(content_hbox), tools_vbox, FALSE, FALSE, 2);

    GtkWidget *size_slider = create_size_slider();
    gtk_box_pack_start(GTK_BOX(tools_vbox), size_slider, FALSE, FALSE, 0);

    GtkWidget *opacity_slider = create_opacity_slider();
    gtk_box_pack_start(GTK_BOX(tools_vbox), opacity_slider, FALSE, FALSE, 0);

    GtkWidget *scroll_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll_window, TRUE);
    gtk_widget_set_vexpand(scroll_window, TRUE);
    gtk_box_pack_start(GTK_BOX(content_hbox), scroll_window, TRUE, TRUE, 0);

    canvas = create_canvas();
    gtk_container_add(GTK_CONTAINER(scroll_window), canvas);

    create_bottom_toolbar(main_vbox);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.example.painteditor", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    cleanup_resources();
    g_object_unref(app);

    return status;
}
