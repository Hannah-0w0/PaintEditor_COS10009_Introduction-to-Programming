#ifndef AUDIO_H
#define AUDIO_H

#include "paint_editor.h"

void init_audio(void);
void cleanup_audio(void);
void play_tool_sound(Tool tool);
void stop_tool_sound(void);

#endif
