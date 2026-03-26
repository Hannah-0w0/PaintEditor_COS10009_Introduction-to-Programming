#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "audio.h"

static Mix_Chunk *pencil_sound = NULL;
static Mix_Chunk *eraser_sound = NULL;
static Mix_Chunk *fill_sound = NULL;
static Mix_Chunk *highlighter_sound = NULL;
static gboolean sound_playing = FALSE;

void init_audio(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        g_print("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        g_print("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return;
    }

    eraser_sound = Mix_LoadWAV("assets/audio/eraser.mp3");
    pencil_sound = Mix_LoadWAV("assets/audio/pencil_scribble.mp3");
    fill_sound = Mix_LoadWAV("assets/audio/fill_bucket.mp3");
    highlighter_sound = Mix_LoadWAV("assets/audio/highlighter.mp3");

    if (!eraser_sound || !pencil_sound || !fill_sound || !highlighter_sound) {
        g_print("Warning: Some sound effects could not be loaded!\n");
    }
}

void cleanup_audio(void) {
    Mix_FreeChunk(eraser_sound);
    Mix_FreeChunk(pencil_sound);
    Mix_FreeChunk(fill_sound);
    Mix_FreeChunk(highlighter_sound);
    Mix_CloseAudio();
    SDL_Quit();
}

void play_tool_sound(Tool tool) {
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
                return;
            default:
                return;
        }

        if (sound) {
            Mix_PlayChannel(-1, sound, -1);
            sound_playing = TRUE;
        }
    }
}

void stop_tool_sound(void) {
    Mix_HaltChannel(-1);
    sound_playing = FALSE;
}
