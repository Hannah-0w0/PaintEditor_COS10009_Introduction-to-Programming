#include <gtk/gtk.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

/**
 * Paint Editor - A GTK-based drawing application
 * 
 * This application provides basic drawing functionality including:
 * - Multiple tools (pencil, eraser, shapes, etc.)
 * - Color selection
 * - Undo/redo capability
 * - Zoom functionality
 * - Sound effects
 */

/**
 * Tool Enumeration
 * Defines all available drawing tools in the application
 * Each tool corresponds to a specific drawing function
 */
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

/**
 * Point Structure
 * Represents a single point in 2D space
 * Used for storing drawing coordinates
 */

// Structure for storing points
typedef struct {
    gdouble x;
    gdouble y;
} Point;

/**
 * DrawingAction Structure
 * Stores all information needed to reproduce a drawing action
 * Used for the undo/redo system
 */

// Structure for storing drawing actions
typedef struct {
    GArray *points;
    GdkRGBA color;
    Tool tool;
    int size;
    gdouble opacity;  // For highlighter
    gchar *text;  // Add this line for text storage
} DrawingAction;


/*
*Global variables maintain the state of the application
*/ 

// Drawing surface and widget variables
static cairo_surface_t *surface = NULL;
static GtkWidget *canvas = NULL;
static GtkWidget *coord_label = NULL;

// Text and drawing state variables
static Tool current_tool = TOOL_PENCIL; // Current tool (default pencil)
static GdkRGBA current_color = {0, 0, 0, 1};  // Current drawing color (default black)
static int brush_size = 10;  // Current brush (default size)
static Point shape_start; // Starting point for shape drawing
static gboolean is_drawing_shape = FALSE; // Shape drawing state

// Highlighter state variables
static GdkRGBA current_highlighter_color = {1, 1, 0, 0.5}; // Default yellow with 50% opacity
static gdouble current_highlighter_opacity = 0.5; // Default opacity

// Cursors
static GdkCursor *pencil_cursor = NULL;
static GdkCursor *eraser_cursor = NULL;
static GdkCursor *highlighter_cursor = NULL;
static GdkCursor *fill_cursor = NULL;
static GdkCursor *crosshair_cursor = NULL;

// Zoom controls
static gdouble zoom_level = 1.0;
static GtkWidget *zoom_scale = NULL;
static GtkWidget *zoom_spin = NULL;

// Undo/Redo stacks
static GList *undo_stack = NULL;
static GList *redo_stack = NULL;
static DrawingAction *current_action = NULL;

// Audio variables
static Mix_Chunk *pencil_sound = NULL;
static Mix_Chunk *eraser_sound = NULL;
static Mix_Chunk *fill_sound = NULL;
static Mix_Chunk *highlighter_sound = NULL;
static gboolean sound_playing = FALSE;

// Function prototypes

// Audio functions
static void init_audio(void);
static void cleanup_audio(void);
static void play_tool_sound(Tool tool);
static void stop_tool_sound(void);

// UI creation and setupfunctions
static void create_tool_cursors(void) ; // Cursor function
static GtkWidget* create_image_from_file(const char* filename);
static GtkWidget* create_toolbar(void);
static GtkWidget* create_tools_toolbar(void);
static GtkWidget* create_canvas(void);
static void create_bottom_toolbar(GtkWidget *vbox);
static GtkWidget* create_size_slider(void);
static GtkWidget* create_opacity_slider(void);

// Drawing and surface functions 
static gboolean create_surface(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static void redraw_all(void);
static void draw_action(cairo_t *cr, DrawingAction *action);
static void draw_brush(GtkWidget *widget, gdouble x, gdouble y);
static void flood_fill(int x, int y, GdkRGBA fill_color);

// Tool and color change state functions
static void change_tool(GtkWidget *widget, gpointer user_data);
static void change_color(GtkColorButton *color_button, gpointer user_data);
static void change_highlighter_color(GtkColorButton *color_button, gpointer user_data);
static void update_tool_size(GtkRange *range, gpointer user_data);
static void update_highlighter_opacity(GtkRange *range, gpointer user_data);

// Zoom control functions
static void update_zoom_level(gdouble new_zoom);
static void update_zoom_from_slider(GtkRange *range, gpointer user_data);
static void update_zoom_from_spin(GtkSpinButton *spin, gpointer user_data);

// Action History functions
static void undo_action(GtkWidget *widget, gpointer user_data);
static void redo_action(GtkWidget *widget, gpointer user_data);
static void free_drawing_action(DrawingAction *action);

// Event handlers
static void on_clear_clicked(GtkWidget *widget, gpointer user_data);
static void on_save_clicked(GtkWidget *widget, gpointer user_data);
static void on_text_entered(GtkEntry *entry, gpointer user_data);
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);

/**
 * Audio System Functions
 */

/**
 * init_audio
 * Initializes the SDL audio system and loads sound effects
 */

// Initialize audio system
static void init_audio(void) {
    // Initialize SDL audio subsystem
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        g_print("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        g_print("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return;
    }

    // Load sound effects
    eraser_sound = Mix_LoadWAV("audio/eraser.mp3");
    pencil_sound = Mix_LoadWAV("audio/pencil_scribble.mp3");
    fill_sound = Mix_LoadWAV("audio/fill_bucket.mp3");
    highlighter_sound = Mix_LoadWAV("audio/highlighter.mp3");

    if (!eraser_sound || !pencil_sound || !fill_sound || !highlighter_sound) {
        g_print("Warning: Some sound effects could not be loaded!\n");
    }
}

/**
 * cleanup_audio
 * Frees all audio resources and shuts down the audio system
 */

// Clean up audio system
static void cleanup_audio(void) {
    Mix_FreeChunk(eraser_sound);
    Mix_FreeChunk(pencil_sound);
    Mix_FreeChunk(fill_sound);
    Mix_FreeChunk(highlighter_sound);
    Mix_CloseAudio();
    SDL_Quit();
}

/**
 * play_tool_sound
 * Plays the appropriate sound effect for the current tool
 * 
 * tool: The tool being used
 */

// Play tool sound
static void play_tool_sound(Tool tool) {
    if (!sound_playing) {
        Mix_Chunk *sound = NULL;
        switch (tool) {
            case TOOL_PENCIL:
                sound = pencil_sound;
                break;
            case TOOL_ERASER:
                sound = eraser_sound;
                break;
            case TOOL_HIGHLIGHTER:
                sound = highlighter_sound;
                break;
            case TOOL_FILL:
                sound = fill_sound;
                Mix_PlayChannel(-1, sound, 0);
                return;  // Return immediately for fill sound
            default:
                return;
        }
        
        if (sound) {
            Mix_PlayChannel(-1, sound, -1);  // -1 for infinite loop
            sound_playing = TRUE;
        }
    }
}

// Stop playing tool sound
static void stop_tool_sound(void) {
    Mix_HaltChannel(-1);
    sound_playing = FALSE;
}




/**
 * UI Creation Functions
 */

/**
 * Tool Cursor
 * Creates custom cursors for different drawing tools
 * Each tool has a unique cursor to indicate the current mode
 */

// Create cursors from images
static void create_tool_cursors(void) {
    GdkDisplay *display = gdk_display_get_default();
    GdkPixbuf *pixbuf;

    // Load pencil cursor
    pixbuf = gdk_pixbuf_new_from_file("icons/pencil.png", NULL);
    if (pixbuf) {
        pencil_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    // Load eraser cursor
    pixbuf = gdk_pixbuf_new_from_file("icons/eraser.png", NULL);
    if (pixbuf) {
        eraser_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    // Load fill cursor
    pixbuf = gdk_pixbuf_new_from_file("icons/fill_bucket.png", NULL);
    if (pixbuf) {
        fill_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    // Load highlighter cursor
    pixbuf = gdk_pixbuf_new_from_file("icons/highlighter.png", NULL);
    if (pixbuf) {
        highlighter_cursor = gdk_cursor_new_from_pixbuf(display, pixbuf, 0, 0);
        g_object_unref(pixbuf);
    }

    // Create crosshair cursor for shapes
    crosshair_cursor = gdk_cursor_new_from_name(display, "crosshair");
}

/**
 * create_image_from_file
 * Loads an image file and creates a GTK image widget from it
 * Used for toolbar icons
 * 
 * filename: Path to the image file
 * return: GtkWidget* containing the loaded image
 */

// Load images for toolbar icons
static GtkWidget* create_image_from_file(const char* filename) {
    GtkWidget* image = gtk_image_new_from_file(filename);
    return image;
}

/*
* Create main toolbar (undo, redo, save, clear)
* return: GtkWidget* containing the configured toolbar
*/

static GtkWidget* create_toolbar(void) {
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);  // Show both icon and text
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);

    // Create undo button
    GtkToolItem *undo_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("edit-undo", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Undo"
    );
    gtk_tool_item_set_tooltip_text(undo_button, "Undo");
    g_signal_connect(undo_button, "clicked", G_CALLBACK(undo_action), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), undo_button, -1);

    // Create redo button
    GtkToolItem *redo_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("edit-redo", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Redo"
    );
    gtk_tool_item_set_tooltip_text(redo_button, "Redo");
    g_signal_connect(redo_button, "clicked", G_CALLBACK(redo_action), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), redo_button, -1);

    // Add separator
    GtkToolItem *sep = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep, -1);

    // Create save button
    GtkToolItem *save_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("document-save", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Save"
    );
    gtk_tool_item_set_tooltip_text(save_button, "Save");
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save_button, -1);

    // Create clear button
    GtkToolItem *clear_button = gtk_tool_button_new(
        gtk_image_new_from_icon_name("edit-clear", GTK_ICON_SIZE_LARGE_TOOLBAR),
        "Clear"
    );
    gtk_tool_item_set_tooltip_text(clear_button, "Clear");
    g_signal_connect(clear_button, "clicked", G_CALLBACK(on_clear_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), clear_button, -1);

    return toolbar;
}

/**
 * create_tools_toolbar
 * Creates the secondary toolbar containing all drawing tools:
 * - Pencil, Eraser, Highlighter | Text | Rectangle, Circle, Square, Triangle, Line | Fill
 * - Color choosers
 * return: GtkWidget* containing the tools toolbar
 */

// Create tools toolbar with horizontal layout
static GtkWidget* create_tools_toolbar(void) {
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);

    // Structure to hold tool definitions
    const struct {
        const char *icon_file;
        const char *tooltip;
        Tool tool;
    } tools[] = {
        {"icons/pencil.png", "Pencil", TOOL_PENCIL},
        {"icons/eraser.png", "Eraser", TOOL_ERASER},
        {"icons/highlighter.png", "Highlighter", TOOL_HIGHLIGHTER},
        {"icons/textbox.png", "Text", TOOL_TEXT},
        {"icons/rectangle.png", "Rectangle", TOOL_RECTANGLE},
        {"icons/triangle.png", "Triangle", TOOL_TRIANGLE},
        {"icons/square.png", "Square", TOOL_SQUARE},
        {"icons/circle.png", "Circle", TOOL_CIRCLE},
        {"icons/line.png", "Line", TOOL_LINE},
        {"icons/fill_bucket.png", "Fill", TOOL_FILL},
        {NULL, NULL, 0}  // Terminator
    };

    // Create tool buttons
    for (int i = 0; tools[i].icon_file != NULL; i++) {
        GtkToolItem *tool_item = gtk_tool_button_new(
            create_image_from_file(tools[i].icon_file),
            tools[i].tooltip
        );
        gtk_tool_item_set_tooltip_text(tool_item, tools[i].tooltip);
        g_signal_connect(tool_item, "clicked", G_CALLBACK(change_tool), 
                        GINT_TO_POINTER(tools[i].tool));
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
    }

    // Add separator
    GtkToolItem *sep = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep, -1);

    // Color chooser
    GtkToolItem *color_item = gtk_tool_item_new();
    GtkWidget *color_button = gtk_color_button_new();
    gtk_container_add(GTK_CONTAINER(color_item), color_button);
    g_signal_connect(color_button, "color-set", G_CALLBACK(change_color), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), color_item, -1);

    // Highlighter color chooser
    GtkToolItem *highlighter_color_item = gtk_tool_item_new();
    GtkWidget *highlighter_button = gtk_color_button_new_with_rgba(&current_highlighter_color);
    gtk_container_add(GTK_CONTAINER(highlighter_color_item), highlighter_button);
    g_signal_connect(highlighter_button, "color-set", G_CALLBACK(change_highlighter_color), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), highlighter_color_item, -1);

    return toolbar;
}


/**
 * create_canvas
 * Creates the main drawing area widget with:
 * - Minimum size requirements
 * - Event handling for mouse and keyboard
 * - Drawing surface initialization
 * 
 * return: GtkWidget* containing the drawing canvas
 */

// Create the main drawing canvas
static GtkWidget* create_canvas(void) {
    GtkWidget *fixed = gtk_fixed_new();
    GtkWidget *drawing_area = gtk_drawing_area_new();

    // set minimum size and make it expand
    gtk_widget_set_size_request(drawing_area, 1850, 1000); // drawing area size x(width),y(height)
    gtk_widget_set_hexpand(drawing_area, TRUE);  // Make canvas expand horizontally
    gtk_widget_set_vexpand(drawing_area, TRUE);  // Make canvas expand vertically

    // This is the key to make it fill the space
    gtk_widget_set_halign(drawing_area, GTK_ALIGN_FILL);
    gtk_widget_set_valign(drawing_area, GTK_ALIGN_FILL);

    // Enable events
    gtk_widget_add_events(drawing_area, 
                         GDK_BUTTON_PRESS_MASK | 
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK | 
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_SCROLL_MASK);

    // Connect signals
    g_signal_connect(drawing_area, "configure-event", G_CALLBACK(create_surface), NULL);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_event), NULL);
    g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_button_press_event), NULL);
    g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify_event), NULL);
    g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(on_button_release_event), NULL);
    g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(on_scroll_event), NULL);

    // Make the drawing area can receive button press events
    gtk_widget_set_can_focus(drawing_area, TRUE);

    gtk_fixed_put(GTK_FIXED(fixed), drawing_area, 0, 0);
    canvas = drawing_area;
    return fixed;
}


/**
 * create_bottom_toolbar
 * Creates the bottom toolbar containing coordinates and zoom controls
 * 
 * vbox: The main container to add the toolbar to
 */

// Create the bottom toolbar with coordinates and zoom controls
static void create_bottom_toolbar(GtkWidget *vbox) {
    GtkWidget *bottom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(bottom_box), 5);
    
    // Create coordinates label with frame
    GtkWidget *coord_frame = gtk_frame_new(NULL);
    coord_label = gtk_label_new("X: 0, Y: 0");
    gtk_container_add(GTK_CONTAINER(coord_frame), coord_label);
    gtk_box_pack_start(GTK_BOX(bottom_box), coord_frame, FALSE, FALSE, 5);

    // Create zoom controls box
    GtkWidget *zoom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(bottom_box), zoom_box, FALSE, FALSE, 0);

    // Add zoom label
    GtkWidget *zoom_label = gtk_label_new("Zoom:");
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_label, FALSE, FALSE, 5);

    // Create zoom scale
    zoom_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 1000, 10);
    gtk_scale_set_draw_value(GTK_SCALE(zoom_scale), FALSE);
    gtk_range_set_value(GTK_RANGE(zoom_scale), 100);
    gtk_widget_set_size_request(zoom_scale, 150, -1);
    g_signal_connect(zoom_scale, "value-changed", G_CALLBACK(update_zoom_from_slider), NULL);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_scale, FALSE, FALSE, 0);

    // Create zoom spin button
    zoom_spin = gtk_spin_button_new_with_range(10, 1000, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_spin), 100);
    gtk_entry_set_width_chars(GTK_ENTRY(zoom_spin), 5);
    g_signal_connect(zoom_spin, "value-changed", G_CALLBACK(update_zoom_from_spin), NULL);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_spin, FALSE, FALSE, 5);

    // Add percentage label
    GtkWidget *percent_label = gtk_label_new("%");
    gtk_box_pack_start(GTK_BOX(zoom_box), percent_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), bottom_box, FALSE, FALSE, 0);
}

/**
 * Create size slider for drawing tools, shapes, text size
 * return: GtkWidget* containing the size slider
 */

// Create size slider for brush/tool size
static GtkWidget* create_size_slider(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
    
    // Create label
    GtkWidget *label = gtk_label_new("Size");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // Create scale
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), brush_size);
    gtk_widget_set_size_request(scale, 30, 200);  // Make slider thinner
    
    g_signal_connect(scale, "value-changed", G_CALLBACK(update_tool_size), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);

    return vbox;
}


/**
 * Create opacity slider for highlighter
 * return: GtkWidget* containing the opacity slider
 */

// Create opacity slider for highlighter
static GtkWidget* create_opacity_slider(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
    
    // Create label
    GtkWidget *label = gtk_label_new("Opacity");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // Create scale
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), current_highlighter_opacity * 100);
    gtk_widget_set_size_request(scale, 30, 200);  // Make slider thinner
    
    g_signal_connect(scale, "value-changed", G_CALLBACK(update_highlighter_opacity), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);

    return vbox;
}



/**
 * Drawing Surface Management
 */

/**
 * create_surface
 * Creates a drawing surface for the canvas
 * 
 * widget: The widget that triggered the event
 * event: The event data
 * data: User data (unused)
 * return: gboolean indicating success
 */

// Create surface for drawing
static gboolean create_surface(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
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

/**
 * redraw_all
 * Redraws the entire canvas
 */

// Redraw the entire canvas
static void redraw_all(void) {
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


/**
 * draw_action
 * Renders a single drawing action on the surface
 * Handles different tools (pencil, shapes, text, etc.)
 * 
 * cr: Cairo context to draw on
 * action: Drawing action to render
 */

// Draw a single action on the surface
static void draw_action(cairo_t *cr, DrawingAction *action) {
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

/**
 * draw_brush
 * Draws a brush stroke on the surface
 * 
 * widget: The widget that triggered the event
 * x: X coordinate
 * y: Y coordinate
 */
static void draw_brush(GtkWidget *widget, gdouble x, gdouble y) {
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


/**
 * Flood Fill Implementation
 * Implements a flood fill (paint bucket) tool using a queue-based approach
 * 
 * x: Starting x coordinate
 * y: Starting y coordinate
 * fill_color: Color to fill with
 */

// Handle fill tool click
static void flood_fill(int x, int y, GdkRGBA fill_color) {
    g_print("flood_fill called: x=%d, y=%d\n", x, y);  // Debug print

    if (!surface){
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
    Point *start = g_new(Point, 1);
    start->x = x;
    start->y = y;
    g_queue_push_tail(queue, start);

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
        Point *p1 = g_new(Point, 1);
        p1->x = px + 1;
        p1->y = py;
        g_queue_push_tail(queue, p1);

        Point *p2 = g_new(Point, 1);
        p2->x = px - 1;
        p2->y = py;
        g_queue_push_tail(queue, p2);

        Point *p3 = g_new(Point, 1);
        p3->x = px;
        p3->y = py + 1;
        g_queue_push_tail(queue, p3);

        Point *p4 = g_new(Point, 1);
        p4->x = px;
        p4->y = py - 1;
        g_queue_push_tail(queue, p4);
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



/**
 * Tool Actions and Color and State Management
 */

// Change the current drawing tool
static void change_tool(GtkWidget *widget, gpointer user_data) {
    Tool new_tool = GPOINTER_TO_INT(user_data);
    g_print("Changing tool to: %d\n", new_tool);
    
    current_tool = new_tool;
    
    // Update cursor based on selected tool
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


// Update the current drawing color
static void change_color(GtkColorButton *color_button, gpointer user_data) {
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_button), &current_color);
    g_print("Color changed to: RGBA(%f, %f, %f, %f)\n",
            current_color.red,
            current_color.green,
            current_color.blue,
            current_color.alpha);
}

// Update the highlighter color
static void change_highlighter_color(GtkColorButton *color_button, gpointer user_data) {
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_button), &current_highlighter_color);
    g_print("Highlighter color changed to: RGBA(%f, %f, %f, %f)\n",
            current_highlighter_color.red,
            current_highlighter_color.green,
            current_highlighter_color.blue,
            current_highlighter_color.alpha);
}

// Update tool size from slider
static void update_tool_size(GtkRange *range, gpointer user_data) {
    brush_size = gtk_range_get_value(range);
    g_print("Tool size changed to: %d\n", brush_size);
}

// Update highlighter opacity from slider
static void update_highlighter_opacity(GtkRange *range, gpointer user_data) {
    current_highlighter_opacity = gtk_range_get_value(range) / 100.0;
    g_print("Highlighter opacity changed to: %f\n", current_highlighter_opacity);
}




/**
 * Zoom Control Functions
 */

// Update zoom level
static void update_zoom_level(gdouble new_zoom) {
    zoom_level = new_zoom / 100.0;
    
    // Update UI controls
    g_signal_handlers_block_by_func(zoom_scale, update_zoom_from_slider, NULL);
    g_signal_handlers_block_by_func(zoom_spin, update_zoom_from_spin, NULL);
    
    gtk_range_set_value(GTK_RANGE(zoom_scale), new_zoom);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_spin), new_zoom);
    
    g_signal_handlers_unblock_by_func(zoom_scale, update_zoom_from_slider, NULL);
    g_signal_handlers_unblock_by_func(zoom_spin, update_zoom_from_spin, NULL);
    
    // Redraw canvas with new zoom level
    if (canvas) {
        gtk_widget_queue_draw(canvas);
    }
}


// Handle zoom slider changes
static void update_zoom_from_slider(GtkRange *range, gpointer user_data) {
    gdouble new_zoom = gtk_range_get_value(range);
    update_zoom_level(new_zoom);
}

// Handle zoom spin button changes
static void update_zoom_from_spin(GtkSpinButton *spin, gpointer user_data) {
    gdouble new_zoom = gtk_spin_button_get_value(spin);
    update_zoom_level(new_zoom);
}


/**
 * Action History Management
 */
/*
Implementation:
- Uses two GList stacks: undo_stack and redo_stack
- Each drawing action is stored as DrawingAction struct containing:
  * points: Array of coordinates (GArray of Point structs)
  * color: Color used (GdkRGBA)
  * tool: Tool type used (Tool enum)
  * size: Brush size used (int)
  * opacity: Opacity value (gdouble)
  * text: Text content if applicable (gchar*)
*/

// Undo last action
static void undo_action(GtkWidget *widget, gpointer user_data) {
    if (undo_stack) {
        DrawingAction *action = (DrawingAction *)g_list_last(undo_stack)->data;
        undo_stack = g_list_remove(undo_stack, action);
        redo_stack = g_list_append(redo_stack, action);
        redraw_all();
    }
}

// Redo last undone action
static void redo_action(GtkWidget *widget, gpointer user_data) {
    if (redo_stack) {
        DrawingAction *action = (DrawingAction *)g_list_last(redo_stack)->data;
        redo_stack = g_list_remove(redo_stack, action);
        undo_stack = g_list_append(undo_stack, action);
        redraw_all();
    }
}

/**
 * free_drawing_action
 * Frees memory allocated for a drawing action
 * Used for cleanup and undo/redo operations
 * 
 * action: The drawing action to free
 */
static void free_drawing_action(DrawingAction *action) {
    if (!action) return;  // Guard against NULL

    if (action->points) {
        g_array_unref(action->points);  // Use unref instead of free
        action->points = NULL;
    }

    // Only free text if it's a text tool action and text exists
    if (action->tool == TOOL_TEXT && action->text) {
        g_free(action->text);
        action->text = NULL;
    }

    g_free(action);
}




/**
 * Event Handlers
 */

/**
 * on_save_clicked
 * Handles the save button click event
 * Opens a file chooser dialog and saves the drawing as PNG
 * 
 * widget: The widget that triggered the event
 * user_data: Additional data (unused)
 */

// Clear the canvas
static void on_clear_clicked(GtkWidget *widget, gpointer user_data) {
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
            // Clear the surface
            cairo_t *cr = cairo_create(surface);
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_paint(cr);
            cairo_destroy(cr);

            // Clear undo/redo stacks
            g_list_free_full(undo_stack, (GDestroyNotify)free_drawing_action);
            g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
            undo_stack = NULL;
            redo_stack = NULL;

            // Reset current action if any
            if (current_action) {
                free_drawing_action(current_action);
                current_action = NULL;
            }

            // Reset drawing state
            is_drawing_shape = FALSE;

            // Redraw the canvas
            gtk_widget_queue_draw(canvas);
        }
    }
}


// Save the drawing to a PNG file
static void on_save_clicked(GtkWidget *widget, gpointer user_data) {
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    gint res;

    
    dialog = gtk_file_chooser_dialog_new("Save File",
                                        GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Save", GTK_RESPONSE_ACCEPT,
                                        NULL);
    chooser = GTK_FILE_CHOOSER(dialog);

    // Set up file chooser properties
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    gtk_file_chooser_set_current_name(chooser, "Untitled.png");

    // Add file filters
    GtkFileFilter *filter_png = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_png, "PNG files");
    gtk_file_filter_add_pattern(filter_png, "*.png");
    gtk_file_chooser_add_filter(chooser, filter_png);

    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All files");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(chooser, filter_all);

    // Handle save operation
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(chooser);
        
        // Ensure surface exists
        if (surface) {
            // Create a new surface for saving (unscaled)
            cairo_surface_t *save_surface = cairo_image_surface_create(
                CAIRO_FORMAT_ARGB32,
                cairo_image_surface_get_width(surface),
                cairo_image_surface_get_height(surface)
            );
            
            // Copy the current surface to the save surface
            cairo_t *cr = cairo_create(save_surface);
            cairo_set_source_surface(cr, surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
            
            // Save to PNG file
            cairo_status_t status = cairo_surface_write_to_png(save_surface, filename);
            if (status != CAIRO_STATUS_SUCCESS) {
                GtkWidget *error_dialog = gtk_message_dialog_new(
                    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Error saving file: %s", // Show error dialog if save fails
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

// Handle text entry
static void on_text_entered(GtkEntry *entry, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(entry);
    if (text && *text) {  // Check if text is not empty
        cairo_t *cr = cairo_create(surface);
        
        // Set text properties
        cairo_set_source_rgba(cr, 
            current_color.red,
            current_color.green,
            current_color.blue,
            current_color.alpha);
        cairo_set_font_size(cr, brush_size);
        
        // Get entry position and draw text
        GtkAllocation allocation;
        gtk_widget_get_allocation(GTK_WIDGET(entry), &allocation);
        cairo_move_to(cr, allocation.x / zoom_level, allocation.y / zoom_level + brush_size);
        cairo_show_text(cr, text);
        
        cairo_destroy(cr);
        gtk_widget_queue_draw(canvas);

        // Create text action for undo stack
        DrawingAction *text_action = g_new(DrawingAction, 1);
        text_action->points = g_array_new(FALSE, FALSE, sizeof(Point));
        Point p = {allocation.x / zoom_level, allocation.y / zoom_level};
        g_array_append_val(text_action->points, p);
        text_action->color = current_color;
        text_action->tool = TOOL_TEXT;
        text_action->size = brush_size;
        text_action->text = g_strdup(text);  // Store the text
        
        // Clear redo stack and add to undo stack
        g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
        redo_stack = NULL;
        undo_stack = g_list_append(undo_stack, text_action);
    }
    
    gtk_widget_destroy(GTK_WIDGET(entry));
}

/**
 * on_button_press_event
 * Handles mouse button press events
 * Initiates drawing actions based on current tool
 * 
 * widget: Widget that received the event
 * event: Mouse button event data
 * user_data: Additional data (unused)
 * return: TRUE to stop event propagation
 */

// Handle draw events
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    if (!surface) return FALSE;
    
    // Apply zoom and draw surface
    cairo_scale(cr, zoom_level, zoom_level);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    
    return FALSE;
}

/**
 * on_button_press_event
 * Handles mouse button press events
 * Initiates drawing actions based on current tool
 * 
 * widget: The widget that triggered the event
 * event: The event data
 * user_data: User data (unused)
 * return: TRUE to stop event propagation
 */

// Handle mouse button press events
static gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->button == GDK_BUTTON_PRIMARY) {
        // Convert coordinates based on zoom level
        gdouble x = event->x / zoom_level;
        gdouble y = event->y / zoom_level;

        g_print("Button press: tool=%d, x=%f, y=%f\n", current_tool, x, y);  // Debug print

        switch (current_tool) {
            case TOOL_FILL:
                // Fill area with current color
                flood_fill(event->x, event->y, current_color);
                play_tool_sound(TOOL_FILL);
                break;

            case TOOL_TEXT: {
                // Create text entry widget at clicked position
                GtkWidget *entry = gtk_entry_new();
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type text...");
                gtk_widget_set_size_request(entry, 100, -1);  // Set a small width for the entry
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
                // Start shape drawing
                shape_start.x = x;
                shape_start.y = y;
                is_drawing_shape = TRUE;
                break;
            
            default: {
                // Start new drawing action
                current_action = g_new(DrawingAction, 1);
                current_action->points = g_array_new(FALSE, FALSE, sizeof(Point));
                current_action->color = (current_tool == TOOL_HIGHLIGHTER) ? 
                                      current_highlighter_color : current_color;
                current_action->tool = current_tool;
                current_action->size = brush_size;
                current_action->opacity = (current_tool == TOOL_HIGHLIGHTER) ? 
                                        current_highlighter_opacity : 1.0;
                
                Point p = {x, y};
                g_array_append_val(current_action->points, p);
                draw_brush(widget, x, y);
                
                // Play appropriate sound for other tools
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

/**
 * on_motion_notify_event
 * Handles mouse motion events during drawing
 * Updates real-time preview of shapes and continues freehand drawing
 * 
 * widget: Widget that received the event
 * event: Mouse motion event data
 * user_data: Additional data (unused)
 * return: TRUE to stop event propagation
 */

static gboolean on_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    gdouble x = event->x / zoom_level;
    gdouble y = event->y / zoom_level;

    // Update coordinates label/ display
    char coord_text[32];
    g_snprintf(coord_text, sizeof(coord_text), "X: %d, Y: %d", (int)event->x, (int)event->y);
    gtk_label_set_text(GTK_LABEL(coord_label), coord_text);

    if (is_drawing_shape) {
        // Redraw shapes in real-time
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
        }
        
        cairo_stroke(cr);
        cairo_destroy(cr);
        gtk_widget_queue_draw(widget);
    } 
    else if (current_action) {
        // Continue drawing action
        Point p = {x, y};
        g_array_append_val(current_action->points, p);
        draw_brush(widget, x, y);
        
        // Continue playing sound if needed
        if (current_tool == TOOL_PENCIL || current_tool == TOOL_ERASER || current_tool == TOOL_HIGHLIGHTER) {
            play_tool_sound(current_tool);
        }
    }
    
    return TRUE;
}

/**
 * on_button_release_event
 * Handles mouse button release events
 * This is crucial for finalizing drawing actions like shapes and lines
 */

// Handle mouse button release events
static gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->button == GDK_BUTTON_PRIMARY) {
        gdouble x = event->x / zoom_level;
        gdouble y = event->y / zoom_level;

        if (is_drawing_shape) {
            // Finalize shape drawing
            is_drawing_shape = FALSE;
            DrawingAction *shape_action = g_new(DrawingAction, 1);
            shape_action->points = g_array_new(FALSE, FALSE, sizeof(Point));
            shape_action->color = current_color;
            shape_action->tool = current_tool;
            shape_action->size = brush_size;

            g_array_append_val(shape_action->points, shape_start);
            Point end_point = {x, y};
            g_array_append_val(shape_action->points, end_point);

            // Clear redo stack and add to undo staplack
            g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
            redo_stack = NULL;
            undo_stack = g_list_append(undo_stack, shape_action);
            
            redraw_all();
        } 
        else if (current_action) {
            // Finalize drawing action
            g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
            redo_stack = NULL;
            undo_stack = g_list_append(undo_stack, current_action);
            current_action = NULL;
        }
        
        // Stop any playing sounds
        stop_tool_sound();
    }
    return TRUE;
}

/**
 * on_scroll_event
 * Handles mouse wheel scrolling for zoom control
 * Allows user to zoom in/out using Ctrl + Mouse Wheel
 */

// Handle scroll events for zooming
static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    if (event->state & GDK_CONTROL_MASK) {
        gdouble new_zoom = zoom_level * 100;
        
        if (event->direction == GDK_SCROLL_UP) {
            new_zoom += 10;  // Zoom in by 10%
        } else if (event->direction == GDK_SCROLL_DOWN) {
            new_zoom -= 10;  // Zoom out by 10%
        }
        
        // Clamp zoom between 10% and 1000%
        new_zoom = CLAMP(new_zoom, 10, 1000);
        update_zoom_level(new_zoom);
        
        return TRUE;
    }
    return FALSE;
}



/**
 * Drawing action Management
 */

// Create a new drawing action
static DrawingAction* create_drawing_action(Tool tool) {
    DrawingAction *action = g_new0(DrawingAction, 1);
    if (!action) return NULL;

    action->points = g_array_new(FALSE, FALSE, sizeof(Point));
    action->tool = tool;
    action->size = brush_size;
    action->text = NULL;
    action->opacity = (tool == TOOL_HIGHLIGHTER) ? current_highlighter_opacity : 1.0;
    
    // Set color based on tool type
    if (tool == TOOL_ERASER) {
        action->color.red = 1.0;
        action->color.green = 1.0;
        action->color.blue = 1.0;
        action->color.alpha = 1.0;
        action->tool = TOOL_ERASER;  // Ensure tool type is set
    } else if (tool == TOOL_HIGHLIGHTER) {
        action->color = current_highlighter_color;
    } else {
        action->color = current_color;
    }
    
    return action;
}


// Add point to current drawing action
static void add_point_to_action(DrawingAction *action, gdouble x, gdouble y) {
    if (action) {
        Point p = {x, y};
        g_array_append_val(action->points, p);
    }
}

// Finalize drawing action
static void finalize_drawing_action(DrawingAction *action) {
    if (action) {
        // Clear redo stack
        g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
        redo_stack = NULL;
        
        // Add to undo stack
        undo_stack = g_list_append(undo_stack, action);
    }
}

// Clear redo stack and add to undo stack
static void add_to_undo_stack(DrawingAction *action) {
    if (!action) return;

    // Clear redo stack
    if (redo_stack) {
        g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
        redo_stack = NULL;
    }

    // Add to undo stack
    undo_stack = g_list_append(undo_stack, action);
}


// Clear the canvas
static void clear_canvas(void) {
    // Clear undo/redo stacks
    if (undo_stack) {
        g_list_free_full(undo_stack, (GDestroyNotify)free_drawing_action);
        undo_stack = NULL;
    }
    if (redo_stack) {
        g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
        redo_stack = NULL;
    }

    // Free current action if any
    if (current_action) {
        free_drawing_action(current_action);
        current_action = NULL;
    }

    // Clear the surface
    if (surface) {
        cairo_t *cr = cairo_create(surface);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);
        cairo_destroy(cr);
        gtk_widget_queue_draw(canvas);
    }
}



/**
 * Resource Management
 * cleanup_resources
 * Frees all application resources during shutdown
 */

static void cleanup_resources(void) {
    // Clean up drawing actions
    if (undo_stack) {
        g_list_free_full(undo_stack, (GDestroyNotify)free_drawing_action);
        undo_stack = NULL;
    }
    if (redo_stack) {
        g_list_free_full(redo_stack, (GDestroyNotify)free_drawing_action);
        redo_stack = NULL;
    }
    if (current_action) {
        free_drawing_action(current_action);
        current_action = NULL;
    }

    // Clean up surface
    if (surface) {
        cairo_surface_destroy(surface);
        surface = NULL;
    }

    // Clean up cursors
    if (pencil_cursor) g_object_unref(pencil_cursor);
    if (eraser_cursor) g_object_unref(eraser_cursor);
    if (fill_cursor) g_object_unref(fill_cursor);
    if (crosshair_cursor) g_object_unref(crosshair_cursor);

    // Clean up audio
    cleanup_audio();
}

/**
 * activate
 * Main application activation function
 * Creates and sets up the main window and all UI elements
 * Application activation handler
 * 
 * app: The GTK application
 * user_data: Additional data (unused)
 */

// In the activate function
static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *main_vbox;    // Main vertical container
    GtkWidget *content_hbox; // Horizontal box for tools and canvas
    GtkWidget *tools_vbox;   // Vertical box for tools and sliders
    GtkWidget *toolbar;      // Main toolbar
    GtkWidget *tools_toolbar; // Tools toolbar

    // Initialize audio system
    init_audio();

    // Create window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Paint Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Create cursors
    create_tool_cursors();

    // Create main vertical container
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // Add main toolbar (undo, redo, save, clear)
    toolbar = create_toolbar();
    gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);

    // Add tools toolbar in its own container for centering
    GtkWidget *tools_toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    tools_toolbar = create_tools_toolbar();
    gtk_box_pack_start(GTK_BOX(tools_toolbar_box), tools_toolbar, TRUE, FALSE, 0);  // Center the toolbar
    gtk_box_pack_start(GTK_BOX(main_vbox), tools_toolbar_box, FALSE, FALSE, 0);

    // Create horizontal box for sliders and canvas
    content_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), content_hbox, TRUE, TRUE, 0);

    // Create vertical box for sliders with minimal width
    tools_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(tools_vbox, 40, -1);  // Minimal width for sliders
    gtk_box_pack_start(GTK_BOX(content_hbox), tools_vbox, FALSE, FALSE, 2);

    // Add size slider
    GtkWidget *size_slider = create_size_slider();
    gtk_box_pack_start(GTK_BOX(tools_vbox), size_slider, FALSE, FALSE, 0);

    // Add opacity slider
    GtkWidget *opacity_slider = create_opacity_slider();
    gtk_box_pack_start(GTK_BOX(tools_vbox), opacity_slider, FALSE, FALSE, 0);

    // Create scrolled window for canvas
    GtkWidget *scroll_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll_window, TRUE);  // Make canvas expand horizontally
    gtk_widget_set_vexpand(scroll_window, TRUE);  // Make canvas expand vertically
    gtk_box_pack_start(GTK_BOX(content_hbox), scroll_window, TRUE, TRUE, 0);

    // Add canvas
    canvas = create_canvas();
    gtk_container_add(GTK_CONTAINER(scroll_window), canvas);

    // Add bottom toolbar
    create_bottom_toolbar(main_vbox);

    // Show all widgets
    gtk_widget_show_all(window);
}


/**
 * main
 * Program entry point
 * Creates and runs the GTK application
 * 
 * argc: Command line argument count
 * argv: Command line arguments
 * return: Application exit status
 */

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Create and initialize GTK application
    app = gtk_application_new("org.example.painteditor", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    // Run application
    status = g_application_run(G_APPLICATION(app), argc, argv);

    // Cleanup and exit
    cleanup_resources(); 
    g_object_unref(app);

    return status;
}

