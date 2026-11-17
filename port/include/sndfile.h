#ifndef _IN_SNDFILE_H
#define _IN_SNDFILE_H

#include <PR/ultratypes.h>

// A loose sound file played over the game's mix, for the title screen's cue.
//
// The game's own sounds are ROM bank samples (src/lib/snd.c). This is a side
// path for one file: it is decoded to mono s16 at the game's output rate and
// mixed into the buffers audioEndFrame() queues, so it follows the same mute,
// surround and recording path as the game's own mix.
//
// The file is looked for beside the game's data (data/sfx/<stem>.wav or .mp3,
// or a mod's sfx/ over it). A build with port/audio/title-screen.mp3 carries
// that sound inside the executable as well, so an update - which replaces only
// the executable - brings a changed one.

// Play stem.wav or stem.mp3, else the built-in title sound. 1 if something was
// started, 0 for the caller to fall back to the bank's own cue.
s32 sndFilePlayCue(const char *stem);

// Play one path (wav or mp3). 1 if it decoded and started, 0 otherwise.
s32 sndFilePlay(const char *path);

// The one-shot's state, read by audio.c as it mixes each output buffer.
s32 sndFileIsPlaying(void);
u32 sndFilePos(void);

// Mix the shot's samples [pos, pos + frames) into an interleaved s16 buffer,
// mono samples to the front pair (FL/FR, or the front pair of a 5.1 layout).
void sndFileMix(s16 *buf, u32 frames, s32 channels, u32 pos);

// Move the playback position on by frames, ending the shot at its end.
void sndFileAdvance(u32 frames);

void sndFileStop(void);

#endif