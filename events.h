#ifndef EVENTS_H
#define EVENTS_H

#include <gtk/gtk.h>

gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);
gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean on_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
void on_clear_clicked(GtkWidget *widget, gpointer user_data);
void on_save_clicked(GtkWidget *widget, gpointer user_data);
void events_cleanup_state(void);

#endif
