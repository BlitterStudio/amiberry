# Step 08: Audio Subsystem Migration

## Objective

Migrate from SDL2's `SDL_OpenAudioDevice`/`SDL_QueueAudio` model to SDL3's `SDL_AudioStream` model. This is the most architecturally different subsystem change in the entire migration — SDL3 completely redesigns audio.

## Prerequisites

- Step 07 complete (event system migrated)

## Files to Modify

| File | Audio Role | Complexity |
|------|-----------|------------|
| `src/sounddep/sound.cpp` | Primary audio output (~22 SDL audio calls) | High |
| `src/osdep/ahi_v1.cpp` | AHI audio emulation (~23 SDL audio calls) | High |
| `src/osdep/ahi_v2.cpp` | AHI v2 audio (~2 SDL audio calls) | Low |
| `src/osdep/cda_play.cpp` | CD audio playback (~2 SDL audio calls) | Low |
| `src/sampler.cpp` | Audio recording/sampling (~6 SDL audio calls) | Medium |

## SDL3 Audio Architecture

SDL3 replaces the old audio model entirely:

**SDL2 Model**:
- `SDL_OpenAudioDevice()` with `SDL_AudioSpec` (callback or queue mode)
- `SDL_QueueAudio()` for push mode
- `SDL_PauseAudioDevice()` / `SDL_LockAudioDevice()`

**SDL3 Model**:
- `SDL_OpenAudioDevice()` returns a device ID (simplified, no spec parameter)
- `SDL_AudioStream` is the central abstraction for audio data flow
- `SDL_OpenAudioDeviceStream()` combines device + stream in one call (convenience)
- Push data via `SDL_PutAudioStreamData()`
- Stream handles format conversion, resampling, and buffering

## Detailed Changes

### 1. Main Audio Output (`sound.cpp`)

**Current pattern** (SDL2 push/queue mode):
```cpp
// SDL2:
SDL_AudioSpec want, have;
want.freq = frequency;
want.format = AUDIO_S16SYS;
want.channels = 2;
want.samples = sndbufsize;
want.callback = NULL;  // Queue mode
SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
SDL_PauseAudioDevice(dev, 0);  // Start playback
// In sound loop:
SDL_QueueAudio(dev, buffer, len);
SDL_GetQueuedAudioSize(dev);
```

**SDL3 equivalent**:
```cpp
// SDL3 — Option A: Stream + device separately
SDL_AudioSpec spec;
spec.freq = frequency;
spec.format = SDL_AUDIO_S16;  // Note: renamed from AUDIO_S16SYS
spec.channels = 2;

SDL_AudioDeviceID dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, &spec);
SDL_BindAudioStream(dev, stream);
SDL_ResumeAudioDevice(dev);

// In sound loop:
SDL_PutAudioStreamData(stream, buffer, len);
SDL_GetAudioStreamQueued(stream);  // Replaces SDL_GetQueuedAudioSize

// Cleanup:
SDL_DestroyAudioStream(stream);
SDL_CloseAudioDevice(dev);
```

```cpp
// SDL3 — Option B: Convenience function (simpler)
SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, frequency };
SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
    SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
SDL_ResumeAudioStreamDevice(stream);

// In sound loop:
SDL_PutAudioStreamData(stream, buffer, len);
SDL_GetAudioStreamQueued(stream);

// Cleanup:
SDL_DestroyAudioStream(stream);  // Also closes device
```

**Recommended**: Option B for simplicity. Update the `sound_dp` struct to hold `SDL_AudioStream*` instead of (or in addition to) `SDL_AudioDeviceID`.

### 2. Audio Callback Mode

If any code uses callbacks (check `sound.cpp` for `want.callback = ...`):

```cpp
// SDL2 callback:
void SDLCALL audio_callback(void* userdata, Uint8* stream, int len) {
    // fill stream buffer
}
want.callback = audio_callback;

// SDL3 equivalent using stream callback:
void SDLCALL audio_get_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    uint8_t buffer[additional_amount];
    // fill buffer
    SDL_PutAudioStreamData(stream, buffer, additional_amount);
}
SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, frequency };
SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
    SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_get_callback, userdata);
```

Alternatively, set a "get" callback on an existing stream:
```cpp
SDL_SetAudioStreamGetCallback(stream, audio_get_callback, userdata);
```

### 3. AHI Audio Emulation (`ahi_v1.cpp`)

AHI uses the same patterns as `sound.cpp` — queue-based audio output plus a recording device:

**Playback**:
```cpp
// Before:
SDL_AudioDeviceID ahi_dev;
SDL_AudioSpec ahi_want, ahi_have;
ahi_dev = SDL_OpenAudioDevice(NULL, 0, &ahi_want, &ahi_have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
SDL_QueueAudio(ahi_dev, data, len);
SDL_PauseAudioDevice(ahi_dev, 0);

// After:
SDL_AudioStream* ahi_stream;
SDL_AudioSpec ahi_spec = { SDL_AUDIO_S16, channels, freq };
ahi_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &ahi_spec, NULL, NULL);
SDL_PutAudioStreamData(ahi_stream, data, len);
SDL_ResumeAudioStreamDevice(ahi_stream);
```

**Recording**:
```cpp
// Before:
SDL_AudioDeviceID ahi_dev_rec;
ahi_dev_rec = SDL_OpenAudioDevice(NULL, SDL_TRUE, &ahi_want, &ahi_have, 0);

// After:
SDL_AudioStream* ahi_rec_stream;
SDL_AudioSpec rec_spec = { SDL_AUDIO_S16, channels, freq };
ahi_rec_stream = SDL_OpenAudioDeviceStream(
    SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &rec_spec, rec_callback, userdata);
```

### 4. Audio Device Locking

```cpp
// Before (SDL2):
SDL_LockAudioDevice(dev);
// modify shared audio state
SDL_UnlockAudioDevice(dev);

// After (SDL3):
SDL_LockAudioStream(stream);
// modify shared audio state
SDL_UnlockAudioStream(stream);
```

### 5. Audio Pause/Resume

```cpp
// Before (SDL2):
SDL_PauseAudioDevice(dev, 1);   // Pause
SDL_PauseAudioDevice(dev, 0);   // Resume

// After (SDL3):
SDL_PauseAudioDevice(dev);      // Pause
SDL_ResumeAudioDevice(dev);     // Resume
// Or if using stream convenience:
SDL_PauseAudioStreamDevice(stream);
SDL_ResumeAudioStreamDevice(stream);
```

### 6. Audio Device Enumeration

```cpp
// Before (SDL2):
int count = SDL_GetNumAudioDevices(0);  // playback devices
for (int i = 0; i < count; i++) {
    const char* name = SDL_GetAudioDeviceName(i, 0);
}

// After (SDL3):
int count;
SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);
for (int i = 0; i < count; i++) {
    const char* name = SDL_GetAudioDeviceName(devices[i]);
}
SDL_free(devices);

// For recording devices:
SDL_AudioDeviceID* rec_devices = SDL_GetAudioRecordingDevices(&count);
```

### 7. Audio Format Constants

| SDL2 | SDL3 |
|------|------|
| `AUDIO_S16SYS` | `SDL_AUDIO_S16` |
| `AUDIO_S16` | `SDL_AUDIO_S16` |
| `AUDIO_F32SYS` | `SDL_AUDIO_F32` |
| `AUDIO_S32` | `SDL_AUDIO_S32` |
| `AUDIO_U8` | `SDL_AUDIO_U8` |

### 8. SDL_AudioSpec Changes

```cpp
// SDL2:
typedef struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint32 size;
    SDL_AudioCallback callback;
    void* userdata;
} SDL_AudioSpec;

// SDL3 (simplified):
typedef struct SDL_AudioSpec {
    SDL_AudioFormat format;
    int channels;
    int freq;
} SDL_AudioSpec;
// Note: callback, userdata, samples, silence, size are all removed from the struct
```

### 9. CD Audio (`cda_play.cpp`)

```cpp
// Before:
SDL_LoadWAV(file, &spec, &buf, &len);
SDL_FreeWAV(buf);

// After:
SDL_LoadWAV(file, &spec, &buf, &len);  // Unchanged but types differ
SDL_free(buf);  // SDL_FreeWAV removed, use SDL_free
```

### 10. Sampler (`sampler.cpp`)

Update recording device opening to use `SDL_AUDIO_DEVICE_DEFAULT_RECORDING` and `SDL_AudioStream`.

## Data Structure Updates

Update Amiberry's internal audio structs to hold `SDL_AudioStream*`:

```cpp
// In sound.cpp or sound.h:
struct sound_dp {
    // Before:
    SDL_AudioDeviceID dev;

    // After:
    SDL_AudioStream* stream;
    // dev is managed internally by the stream when using SDL_OpenAudioDeviceStream
};
```

Similarly in `ahi_v1.cpp`:
```cpp
// Before:
static SDL_AudioDeviceID ahi_dev;
static SDL_AudioDeviceID ahi_dev_rec;

// After:
static SDL_AudioStream* ahi_stream;
static SDL_AudioStream* ahi_rec_stream;
```

## Search Patterns

```bash
grep -rn 'SDL_OpenAudioDevice\|SDL_CloseAudioDevice' src/
grep -rn 'SDL_QueueAudio\|SDL_GetQueuedAudioSize\|SDL_ClearQueuedAudio' src/
grep -rn 'SDL_PauseAudioDevice\|SDL_LockAudioDevice\|SDL_UnlockAudioDevice' src/
grep -rn 'SDL_AudioSpec\|SDL_AudioDeviceID' src/
grep -rn 'AUDIO_S16\|AUDIO_F32\|AUDIO_U8' src/
grep -rn 'SDL_GetNumAudioDevices\|SDL_GetAudioDeviceName' src/
grep -rn 'SDL_LoadWAV\|SDL_FreeWAV' src/
grep -rn 'SDL_AUDIO_ALLOW' src/
```

## Acceptance Criteria

1. Main audio output works (no crackling, stuttering, or silence)
2. AHI audio emulation functions correctly (Amiga software using AHI produces sound)
3. Audio recording/sampling works (sampler input)
4. CD audio playback works
5. Audio device enumeration returns valid device names
6. Audio pause/resume works without glitches
7. No audio buffer underruns or overruns during normal operation
8. Audio latency is comparable to SDL2 build (not noticeably worse)
9. Multiple audio streams (main + AHI) can operate simultaneously
10. Clean shutdown without audio device errors
