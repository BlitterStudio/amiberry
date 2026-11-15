/*
 * Calls for programs that link the Amiberry core in (libretro/Makefile with
 * STATIC_LINKING=1) and drive it through the libretro API, beyond that API.
 */
#ifndef AMIBERRY_EMBED_H
#define AMIBERRY_EMBED_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Arguments added to the end of the command line the core passes to
 * Amiberry when it starts the emulation, after the ones it makes from its
 * core options and the content, so they win over those. For settings
 * Amiberry only reads at start, e.g. { "-s", "bsdsocket_emu=true" }.
 *
 * Set them before retro_load_game(); they apply to every start until set
 * again. The strings are copied. ARGC 0 clears them.
 */
void amiberry_libretro_set_extra_args(int argc, const char *const *argv);

/*
 * Sound: the core renders at the rate retro_get_system_av_info() reports,
 * 44100 Hz unless the amiberry_audio_rate core option asks for 48000. The
 * option is read in retro_load_game(). A program that plays the samples
 * without resampling has to set it to its device's rate; otherwise the
 * sound runs at the wrong speed against the video.
 */

#ifdef __cplusplus
}
#endif

#endif /* AMIBERRY_EMBED_H */
