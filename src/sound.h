/* src/sound.h -- tones through audio.device: the Bell as a sound, ANSI music.
 * A channel is allocated on the first tone and kept until Sound_Close(); a
 * tune is queued note by note and plays while DCTelnet goes on. */
#ifndef SOUND_H
#define SOUND_H

#include <exec/types.h>
#include "ansimusic.h"

void Sound_Play(const struct Note *notes, int count);
void Sound_Beep(void);
void Sound_Close(void);

#endif /* SOUND_H */
