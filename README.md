# Paint Editor

A lightweight, feature-rich painting application built in C with GTK-3.0. Create digital artwork with multiple drawing tools, undo/redo functionality, zoom controls, and audio feedback.

## Features

### Drawing Tools
- **Pencil** - Freehand drawing with variable brush sizes and opacity
- **Eraser** - Remove content from canvas with adjustable size
- **Highlighter** - Semi-transparent marker tool with customizable opacity
- **Text** - Add text directly to the canvas
- **Shapes** - Draw rectangles, triangles, squares, circles, and lines
- **Fill Bucket** - Flood fill tool to paint connected regions with a single color
- **Color Picker** - Sample colors directly from the canvas

### Core Functionality
- **Undo/Redo** - Full action history with stack-based state management
- **Zoom Control** - Zoom in/out with mouse wheel (Ctrl+Scroll) and spin controls
- **Color Selection** - GTK color chooser for unlimited color options
- **Brush Size Slider** - Adjust brush size from thin to thick strokes
- **Opacity Control** - Control transparency of brush and highlighter
- **Save as PNG** - Export your artwork as PNG images
- **Canvas Clearing** - Clear the entire canvas with confirmation dialog
- **Audio Feedback** - Sound effects for each drawing tool
- **Coordinate Display** - Real-time mouse coordinate tracking on canvas

## Requirements

### Windows (MSYS2/MinGW64)

You'll need:
- **GCC compiler** (mingw64 version via MSYS2)
- **GTK-3.0** development libraries
- **Cairo** graphics library
- **GLib** core library
- **SDL2 & SDL2_mixer** for audio support

### Installation on Windows

1. **Install MSYS2** from [https://www.msys2.org/](https://www.msys2.org/)

2. **Open MSYS2 MinGW64 terminal** and run:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-cairo mingw-w64-x86_64-glib2
   pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer
   pacman -S mingw-w64-x86_64-gcc
   ```

3. **Clone the repository** and navigate to the directory:
   ```bash
   git clone https://github.com/yourusername/PaintEditor_C.git
   cd PaintEditor_C
   ```

## Compilation

The project includes all source files needed to compile. Use the following command:

```bash
gcc -std=c11 -Wall -Wextra \
  paint_editor.c ui.c events.c drawing.c audio.c history.c \
  `pkg-config --cflags gtk+-3.0 cairo glib-2.0` \
  `pkg-config --libs gtk+-3.0 cairo glib-2.0 SDL2 SDL2_mixer` \
  -o paint_editor.exe
```

Or on Linux/macOS:
```bash
gcc -std=c11 -Wall -Wextra \
  paint_editor.c ui.c events.c drawing.c audio.c history.c \
  `pkg-config --cflags gtk+-3.0 cairo glib-2.0` \
  `pkg-config --libs gtk+-3.0 cairo glib-2.0 SDL2 SDL2_mixer` \
  -o paint_editor
```

## Running the Application

After compilation, run:

**Windows:**
```bash
.\paint_editor.exe
```

**Linux/macOS:**
```bash
./paint_editor
```

Or double-click the executable from your file manager.

## Project Structure

### Source Files

| File | Purpose |
|------|---------|
| `paint_editor.c/.h` | Application bootstrap, lifecycle management, and shared state definitions |
| `ui.c/.h` | UI widget creation: toolbars, sliders, canvas, cursors, and zoom controls |
| `events.c/.h` | Input event handlers (mouse, scroll, text) and action creation logic |
| `drawing.c/.h` | Rendering engine: surface management, brush strokes, and flood fill algorithm |
| `history.c/.h` | Undo/redo stack management and action memory cleanup |
| `audio.c/.h` | Audio initialization, sound playback, and SDL_mixer integration |

### Asset Directories

| Directory | Contents |
|-----------|----------|
| `audio/` | Tool sound effects (MP3 format) for pencil, eraser, highlighter, brush, and fill |
| `icons/` | PNG tool icons and cursors |

### Shared State

All modules share application state through `paint_editor.h` extern declarations:
- `surface` - Cairo drawing surface
- `canvas` - GTK canvas widget
- `current_tool` - Active drawing tool
- `current_color` - Selected color (RGBA)
- `brush_size` - Brush diameter in pixels
- `current_highlighter_color` - Highlighter color (RGBA)
- `current_highlighter_opacity` - Highlighter transparency
- `zoom_level` - Canvas zoom factor
- `undo_stack` - Drawing action history
- `redo_stack` - Undone actions for redo

## Architecture

The application uses a **modular architecture** with clean separation of concerns:

```
       paint_editor (main)
            |
    +-------+-------+-------+-------+-------+
    |       |       |       |       |       |
   ui    events  drawing  audio  history  [shared state]
```

- **Main module** coordinates application lifecycle and manages shared state
- **UI module** constructs all widgets (toolbars, sliders, canvas)
- **Events module** handles user input and creates drawing actions
- **Drawing module** renders actions to canvas using Cairo
- **Audio module** plays sound effects via SDL_mixer
- **History module** manages undo/redo stacks

Each module exposes a public interface via header files while keeping implementation details private.

## Dependencies & Libraries

### Core Graphics
- **GTK-3.0** - Cross-platform GUI toolkit
- **Cairo** - 2D graphics library for rendering

### System Libraries
- **GLib-2.0** - Core data structures and utilities
- **GObject** - Object system for GTK

### Audio
- **SDL2** - Simple DirectMedia Layer for cross-platform support
- **SDL2_mixer** - Audio mixing for sound playback

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Z` | Undo last action |
| `Ctrl+Y` | Redo last undone action |
| `Ctrl+Scroll` | Zoom in/out on canvas |
| `Ctrl+S` | Save canvas as PNG |
| `Ctrl+N` | Clear canvas (with confirmation) |

## Features in Detail

### Pencil Tool
- Freehand drawing with smooth lines
- Adjustable brush size
- Variable opacity (alpha blending)
- Customizable color selection
- Sound effect on drawing

### Eraser Tool
- Remove pixels from canvas
- Variable eraser size
- Non-destructive (maintains PNG transparency)

### Highlighter
- Semi-transparent marker strokes
- Independent color from main brush
- Opacity slider for highlight intensity
- Blends with underlying content

### Shapes
- **Rectangle** - Filled or outlined rectangles
- **Triangle** - Three-point triangles
- **Square** - Perfect squares
- **Circle** - Filled circles
- **Line** - Straight lines with any thickness

### Fill Bucket
- Flood fill algorithm with color spreading
- Queue-based pixel exploration
- Connected region detection
- Works with any color on canvas

### Text Tool
- Click on canvas to place text
- GTK text input dialog
- Selectable font and size
- Anti-aliased rendering via Cairo

## Troubleshooting

### Application won't start
- Ensure all libraries are installed: `pkg-config --modversion gtk+-3.0`
- Check that audio files exist in `audio/` directory
- Verify icon files exist in `icons/` directory

### No sound effects
- Confirm `audio/*.mp3` files are in the correct directory
- Check SDL_mixer initialization with: `get_errors` on audio.c

### GTK not found during compilation
- Re-run MSYS2 package installation commands
- Update PATH to include MinGW64 bin directory

### Compilation errors
- Ensure C11 standard support: use `-std=c11` flag
- Verify all source files are present
- Check for missing header files

## Future Enhancements

- Layers support for non-destructive editing
- Selection tools (rectangular, elliptical)
- Transform tools (rotate, scale, flip)
- Additional brush styles (spray, pattern)
- Gradient fills
- Animation/frame support
- Adjustable canvas size
- Recent files menu

## License

This project is provided as-is for educational and personal use.

## Contributing

Contributions welcome! Feel free to fork, submit issues, and create pull requests.

---

**Made with C, GTK, and Cairo** ✨
