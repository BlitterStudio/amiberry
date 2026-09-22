#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "videograb.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef AMIBERRY_WITH_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#endif

#ifdef USE_SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_camera.h>
#include <SDL3/SDL_surface.h>
#endif

#define BI_RGB 0

struct AviFrame {
    char id[4];
    long data_pos;
    uae_u32 size;
};

struct AviIndexEntry {
    char id[4];
    uae_u32 offset;
    uae_u32 size;
};

struct AviInfo {
    int stream_count = 0;
    int video_stream = -1;
    int width = 0;
    int height = 0;
    bool top_down = false;
    uae_u32 microsec_per_frame = 40000;
    uae_u32 stream_scale = 0;
    uae_u32 stream_rate = 0;
    long movi_data_start = 0;
    long movi_data_end = 0;
    std::vector<AviFrame> frames;
    std::vector<AviIndexEntry> index;
};

enum VideoGrabMode {
    VIDEOGRAB_NONE,
    VIDEOGRAB_AVI,
    VIDEOGRAB_FFMPEG,
    VIDEOGRAB_CAMERA
};

static VideoGrabMode videograb_mode = VIDEOGRAB_NONE;
static FILE *avi_file;
static std::vector<AviFrame> avi_frames;
static std::vector<long> frame_buffer;
static int video_width;
static int video_height;
static int video_paused = -1;
static int audio_chflags;
static int audio_volume;
static uae_s64 current_frame;
static uae_s64 play_base_frame;
static uae_u32 frame_usec = 40000;
static std::chrono::steady_clock::time_point play_base_time;
static bool video_initialized;
static bool avi_top_down;
static uae_s64 loaded_frame = -1;
static bool audio_muted;

#ifdef USE_SDL3
static SDL_Camera *camera;
static bool camera_sdl_initialized;
static bool camera_permission_denied_logged;
#endif

#ifdef AMIBERRY_WITH_FFMPEG
static AVFormatContext *ffmpeg_format;
static AVCodecContext *ffmpeg_video_codec;
static AVCodecContext *ffmpeg_audio_codec;
static AVFrame *ffmpeg_video_frame;
static AVFrame *ffmpeg_audio_frame;
static AVPacket *ffmpeg_packet;
static SwsContext *ffmpeg_sws;
static SwrContext *ffmpeg_swr;
static int ffmpeg_video_stream_index = -1;
static int ffmpeg_audio_stream_index = -1;
static AVRational ffmpeg_frame_rate = { 25, 1 };
static uae_s64 ffmpeg_duration_frames;
static uae_s64 ffmpeg_decoded_frame = -1;
static uae_s64 ffmpeg_audio_discard_until_frame = -1;
static bool ffmpeg_packet_pending;
static int audio_master_volume = 100;
static bool audio_master_muted;
#ifdef USE_SDL3
static SDL_AudioStream *ffmpeg_audio_stream;
static bool ffmpeg_audio_sdl_initialized;
#endif
static float ffmpeg_audio_gain(void)
{
    if (audio_muted || audio_master_muted || !audio_chflags) {
        return 0.0f;
    }
    const float source_gain = std::max(0, std::min(100, audio_volume)) / 100.0f;
    const float master_gain = std::max(0, std::min(100, audio_master_volume)) / 100.0f;
    return source_gain * master_gain;
}

static void ffmpeg_update_audio_gain(void)
{
#ifdef USE_SDL3
    if (ffmpeg_audio_stream && !SDL_SetAudioStreamGain(ffmpeg_audio_stream, ffmpeg_audio_gain())) {
        write_log(_T("VIDEOGRAB: SDL audio gain update failed: %s\n"), SDL_GetError());
    }
#endif
}
#endif

static bool fourcc_equals(const char id[4], const char *value)
{
    return std::memcmp(id, value, 4) == 0;
}

static uae_u32 read_le32(const uae_u8 *p)
{
    return (uae_u32)p[0] |
        ((uae_u32)p[1] << 8) |
        ((uae_u32)p[2] << 16) |
        ((uae_u32)p[3] << 24);
}

static uae_s32 read_le_s32(const uae_u8 *p)
{
    return (uae_s32)read_le32(p);
}

static uae_u16 read_le16(const uae_u8 *p)
{
    return (uae_u16)(p[0] | (p[1] << 8));
}

static bool read_exact(FILE *f, void *data, size_t size)
{
    return size == 0 || std::fread(data, 1, size, f) == size;
}

static bool read_chunk_header(FILE *f, char id[4], uae_u32 *size)
{
    uae_u8 bytes[4];

    if (!read_exact(f, id, 4) || !read_exact(f, bytes, sizeof bytes)) {
        return false;
    }
    *size = read_le32(bytes);
    return true;
}

static long file_size(FILE *f)
{
    const long pos = std::ftell(f);
    if (pos < 0 || std::fseek(f, 0, SEEK_END) != 0) {
        return 0;
    }
    const long size = std::ftell(f);
    std::fseek(f, pos, SEEK_SET);
    return size;
}

static bool seek_file(FILE *f, long pos)
{
    return pos >= 0 && std::fseek(f, pos, SEEK_SET) == 0;
}

static bool read_chunk_data(FILE *f, uae_u32 size, std::vector<uae_u8> *data)
{
    data->resize(size);
    return read_exact(f, data->data(), size);
}

static bool valid_video_chunk_id(const AviInfo &info, const char id[4])
{
    if (info.video_stream < 0) {
        return false;
    }
    if (id[0] < '0' || id[0] > '9' || id[1] < '0' || id[1] > '9') {
        return false;
    }
    const int stream = (id[0] - '0') * 10 + (id[1] - '0');
    return stream == info.video_stream && id[2] == 'd' &&
        (id[3] == 'b' || id[3] == 'c');
}

static void parse_avih(const std::vector<uae_u8> &data, AviInfo *info)
{
    if (data.size() < 20) {
        return;
    }
    const uae_u32 usec = read_le32(data.data());
    if (usec > 0) {
        info->microsec_per_frame = usec;
    }
}

static void parse_strh(const std::vector<uae_u8> &data, int stream_index, AviInfo *info)
{
    if (stream_index < 0 || data.size() < 36 || !fourcc_equals((const char *)data.data(), "vids")) {
        return;
    }
    if (info->video_stream >= 0) {
        return;
    }
    info->video_stream = stream_index;
    info->stream_scale = read_le32(data.data() + 20);
    info->stream_rate = read_le32(data.data() + 24);
}

static void parse_strf(const std::vector<uae_u8> &data, int stream_index, AviInfo *info)
{
    if (stream_index != info->video_stream || data.size() < 40) {
        return;
    }

    const uae_s32 width = read_le_s32(data.data() + 4);
    const uae_s32 signed_height = read_le_s32(data.data() + 8);
    const uae_u16 planes = read_le16(data.data() + 12);
    const uae_u16 bit_count = read_le16(data.data() + 14);
    const uae_u32 compression = read_le32(data.data() + 16);
    if (width <= 0 || signed_height == 0 || planes != 1 ||
        bit_count != 24 || compression != BI_RGB) {
        return;
    }

    info->width = width;
    info->height = signed_height < 0 ? -signed_height : signed_height;
    info->top_down = signed_height < 0;
}

static void parse_idx1(const std::vector<uae_u8> &data, AviInfo *info)
{
    const size_t count = data.size() / 16;
    for (size_t i = 0; i < count; i++) {
        const uae_u8 *entry = data.data() + i * 16;
        AviIndexEntry idx;
        std::memcpy(idx.id, entry, 4);
        idx.offset = read_le32(entry + 8);
        idx.size = read_le32(entry + 12);
        info->index.push_back(idx);
    }
}

static bool parse_avi_range(FILE *f, long end, int stream_index, AviInfo *info)
{
    while (std::ftell(f) >= 0 && std::ftell(f) + 8 <= end) {
        char id[4];
        uae_u32 size;
        const long chunk_start = std::ftell(f);
        if (!read_chunk_header(f, id, &size)) {
            return false;
        }

        const long data_start = std::ftell(f);
        const long chunk_end = data_start + (long)size + (size & 1);
        if (chunk_end < data_start || chunk_end > end + 1) {
            return false;
        }

        if (fourcc_equals(id, "LIST") && size >= 4) {
            char list_type[4];
            if (!read_exact(f, list_type, sizeof list_type)) {
                return false;
            }
            const long list_data_start = std::ftell(f);
            const long list_data_end = data_start + (long)size;
            if (fourcc_equals(list_type, "strl")) {
                const int child_stream = info->stream_count++;
                if (!parse_avi_range(f, list_data_end, child_stream, info)) {
                    return false;
                }
            } else if (fourcc_equals(list_type, "movi")) {
                info->movi_data_start = list_data_start;
                info->movi_data_end = list_data_end;
            } else if (!parse_avi_range(f, list_data_end, stream_index, info)) {
                return false;
            }
        } else {
            std::vector<uae_u8> data;
            if (fourcc_equals(id, "avih")) {
                if (!read_chunk_data(f, size, &data)) {
                    return false;
                }
                parse_avih(data, info);
            } else if (fourcc_equals(id, "strh")) {
                if (!read_chunk_data(f, size, &data)) {
                    return false;
                }
                parse_strh(data, stream_index, info);
            } else if (fourcc_equals(id, "strf")) {
                if (!read_chunk_data(f, size, &data)) {
                    return false;
                }
                parse_strf(data, stream_index, info);
            } else if (fourcc_equals(id, "idx1")) {
                if (!read_chunk_data(f, size, &data)) {
                    return false;
                }
                parse_idx1(data, info);
            }
        }

        if (!seek_file(f, chunk_end)) {
            return false;
        }
        if (std::ftell(f) <= chunk_start) {
            return false;
        }
    }
    return true;
}

static bool indexed_chunk_data_pos(FILE *f, const AviInfo &info,
    const AviIndexEntry &idx, long *data_pos)
{
    const long bases[] = {
        info.movi_data_start,
        info.movi_data_start - 4,
        0
    };

    for (long base : bases) {
        const long chunk_pos = base + (long)idx.offset;
        if (!seek_file(f, chunk_pos)) {
            continue;
        }
        char id[4];
        uae_u32 size;
        if (read_chunk_header(f, id, &size) &&
            std::memcmp(id, idx.id, 4) == 0 && size >= idx.size) {
            *data_pos = chunk_pos + 8;
            return true;
        }
    }
    return false;
}

static void build_indexed_frames(FILE *f, AviInfo *info)
{
    for (const AviIndexEntry &idx : info->index) {
        if (!valid_video_chunk_id(*info, idx.id) || idx.size == 0) {
            continue;
        }
        long data_pos;
        if (!indexed_chunk_data_pos(f, *info, idx, &data_pos)) {
            continue;
        }
        AviFrame frame;
        std::memcpy(frame.id, idx.id, 4);
        frame.data_pos = data_pos;
        frame.size = idx.size;
        info->frames.push_back(frame);
    }
}

static bool scan_movi_frames(FILE *f, AviInfo *info, long start, long end)
{
    if (!seek_file(f, start)) {
        return false;
    }

    while (std::ftell(f) >= 0 && std::ftell(f) + 8 <= end) {
        char id[4];
        uae_u32 size;
        const long chunk_start = std::ftell(f);
        if (!read_chunk_header(f, id, &size)) {
            return false;
        }
        const long data_start = std::ftell(f);
        const long chunk_end = data_start + (long)size + (size & 1);
        if (chunk_end < data_start || chunk_end > end + 1) {
            return false;
        }
        if (fourcc_equals(id, "LIST") && size >= 4) {
            char list_type[4];
            if (!read_exact(f, list_type, sizeof list_type)) {
                return false;
            }
            if (fourcc_equals(list_type, "rec ")) {
                const long list_data_start = std::ftell(f);
                const long list_data_end = data_start + (long)size;
                if (!scan_movi_frames(f, info, list_data_start, list_data_end)) {
                    return false;
                }
            }
        } else if (valid_video_chunk_id(*info, id) && size > 0) {
            AviFrame frame;
            std::memcpy(frame.id, id, 4);
            frame.data_pos = data_start;
            frame.size = size;
            info->frames.push_back(frame);
        }
        if (!seek_file(f, chunk_end)) {
            return false;
        }
        if (std::ftell(f) <= chunk_start) {
            return false;
        }
    }
    return true;
}

static bool parse_avi_file(FILE *f, AviInfo *info)
{
    char id[4];
    uae_u32 riff_size;
    char type[4];

    if (!seek_file(f, 0) || !read_chunk_header(f, id, &riff_size) ||
        !fourcc_equals(id, "RIFF") || !read_exact(f, type, sizeof type) ||
        !fourcc_equals(type, "AVI ")) {
        return false;
    }

    const long end = std::min(file_size(f), (long)riff_size + 8);
    if (end <= 12 || !parse_avi_range(f, end, -1, info)) {
        return false;
    }

    if (info->stream_rate > 0 && info->stream_scale > 0) {
        const double usec = 1000000.0 * (double)info->stream_scale /
            (double)info->stream_rate;
        if (usec >= 1.0) {
            info->microsec_per_frame = (uae_u32)usec;
        }
    }

    if (!info->index.empty()) {
        build_indexed_frames(f, info);
    }
    if (info->frames.empty() && info->movi_data_start > 0 &&
        info->movi_data_end > info->movi_data_start) {
        scan_movi_frames(f, info, info->movi_data_start, info->movi_data_end);
    }

    return info->width > 0 && info->height > 0 && !info->frames.empty();
}

static uae_u8 *frame_bytes(void)
{
    return reinterpret_cast<uae_u8 *>(frame_buffer.data());
}

static bool resize_frame_buffer(size_t bytes)
{
    const size_t longs = (bytes + sizeof(long) - 1) / sizeof(long);
    try {
        frame_buffer.resize(longs);
    } catch (...) {
        frame_buffer.clear();
        return false;
    }
    return true;
}

static uae_s64 normalized_frame(uae_s64 frame)
{
    const uae_s64 count = (uae_s64)avi_frames.size();
    if (count <= 0) {
        return 0;
    }
    frame %= count;
    if (frame < 0) {
        frame += count;
    }
    return frame;
}

static uae_s64 current_avi_frame(void)
{
    if (video_paused > 0) {
        return normalized_frame(current_frame);
    }

    const auto now = std::chrono::steady_clock::now();
    const uae_s64 elapsed = (uae_s64)std::chrono::duration_cast
        <std::chrono::microseconds>(now - play_base_time).count();
    const uae_s64 delta = frame_usec > 0 ? elapsed / frame_usec : 0;
    current_frame = normalized_frame(play_base_frame + delta);
    return current_frame;
}

static bool read_avi_frame(uae_s64 frame)
{
    if (!avi_file || avi_frames.empty() || video_width <= 0 || video_height <= 0) {
        return false;
    }

    frame = normalized_frame(frame);
    if (loaded_frame == frame && !frame_buffer.empty()) {
        return true;
    }

    const AviFrame &avi_frame = avi_frames[(size_t)frame];
    std::vector<uae_u8> data(avi_frame.size);
    if (!seek_file(avi_file, avi_frame.data_pos) ||
        !read_exact(avi_file, data.data(), data.size())) {
        write_log(_T("VIDEOGRAB: failed reading AVI frame %lld\n"), frame);
        return false;
    }

    const size_t row_bytes = (size_t)video_width * 3;
    const size_t padded_row_bytes = (row_bytes + 3) & ~(size_t)3;
    size_t source_pitch = padded_row_bytes;
    if (data.size() < source_pitch * (size_t)video_height) {
        if (data.size() < row_bytes * (size_t)video_height) {
            write_log(_T("VIDEOGRAB: short AVI frame %lld\n"), frame);
            return false;
        }
        source_pitch = row_bytes;
    }

    if (!resize_frame_buffer(row_bytes * (size_t)video_height)) {
        return false;
    }

    uae_u8 *dst = frame_bytes();
    for (int y = 0; y < video_height; y++) {
        const int source_y = avi_top_down ? video_height - 1 - y : y;
        const uae_u8 *src = data.data() + (size_t)source_y * source_pitch;
        std::memcpy(dst + (size_t)y * row_bytes, src, row_bytes);
    }

    loaded_frame = frame;
    return true;
}

static bool init_avi_videograb(const TCHAR *filename)
{
    AviInfo info;

    avi_file = std::fopen(filename, "rb");
    if (!avi_file) {
        write_log(_T("VIDEOGRAB: can't open '%s'\n"), filename);
        return false;
    }

    if (!parse_avi_file(avi_file, &info)) {
        write_log(_T("VIDEOGRAB: unsupported AVI '%s' (need 24-bit uncompressed video)\n"),
            filename);
        std::fclose(avi_file);
        avi_file = NULL;
        return false;
    }

    avi_frames = info.frames;
    video_width = info.width;
    video_height = info.height;
    avi_top_down = info.top_down;
    frame_usec = info.microsec_per_frame ? info.microsec_per_frame : 40000;
    current_frame = 0;
    play_base_frame = 0;
    play_base_time = std::chrono::steady_clock::now();
    video_paused = 0;
    loaded_frame = -1;

    write_log(_T("VIDEOGRAB: playing '%s', %dx%d, %d frames, %.3f fps\n"),
        filename, video_width, video_height, (int)avi_frames.size(),
        1000000.0 / (double)frame_usec);
    return true;
}

#ifdef AMIBERRY_WITH_FFMPEG
static const char *ffmpeg_error_text(int err, char *buffer, size_t size)
{
    if (av_strerror(err, buffer, size) < 0) {
        std::snprintf(buffer, size, "error %d", err);
    }
    return buffer;
}

static AVRational ffmpeg_frame_time_base(void)
{
    AVRational tb = { ffmpeg_frame_rate.den, ffmpeg_frame_rate.num };
    if (tb.num <= 0 || tb.den <= 0) {
        tb.num = 1;
        tb.den = 25;
    }
    return tb;
}

static uae_s64 normalized_ffmpeg_frame(uae_s64 frame)
{
    if (ffmpeg_duration_frames <= 0) {
        return frame < 0 ? 0 : frame;
    }
    frame %= ffmpeg_duration_frames;
    if (frame < 0) {
        frame += ffmpeg_duration_frames;
    }
    return frame;
}

static uae_s64 ffmpeg_frame_from_pts(uae_s64 pts)
{
    if (pts == AV_NOPTS_VALUE || !ffmpeg_format || ffmpeg_video_stream_index < 0) {
        return ffmpeg_decoded_frame + 1;
    }
    AVStream *stream = ffmpeg_format->streams[ffmpeg_video_stream_index];
    const uae_s64 start_time = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
    return av_rescale_q(pts - start_time, stream->time_base, ffmpeg_frame_time_base());
}

static uae_s64 ffmpeg_timestamp_from_frame(uae_s64 frame)
{
    if (!ffmpeg_format || ffmpeg_video_stream_index < 0) {
        return 0;
    }
    AVStream *stream = ffmpeg_format->streams[ffmpeg_video_stream_index];
    const uae_s64 start_time = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
    return start_time + av_rescale_q(frame, ffmpeg_frame_time_base(), stream->time_base);
}

static bool ffmpeg_discard_audio_packet(const AVPacket *packet)
{
    if (ffmpeg_audio_discard_until_frame < 0 || !packet || !ffmpeg_format ||
        ffmpeg_audio_stream_index < 0) {
        return false;
    }

    const uae_s64 timestamp = packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts;
    if (timestamp == AV_NOPTS_VALUE) {
        ffmpeg_audio_discard_until_frame = -1;
        return false;
    }

    AVStream *stream = ffmpeg_format->streams[ffmpeg_audio_stream_index];
    const uae_s64 start_time = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
    if (av_compare_ts(timestamp - start_time, stream->time_base,
        ffmpeg_audio_discard_until_frame, ffmpeg_frame_time_base()) < 0) {
        return true;
    }

    ffmpeg_audio_discard_until_frame = -1;
    return false;
}

static uae_s64 ffmpeg_current_frame(void)
{
    if (video_paused > 0) {
        return normalized_ffmpeg_frame(current_frame);
    }

    const auto now = std::chrono::steady_clock::now();
    const uae_s64 elapsed = (uae_s64)std::chrono::duration_cast
        <std::chrono::microseconds>(now - play_base_time).count();
    const uae_s64 delta = frame_usec > 0 ? elapsed / frame_usec : 0;
    current_frame = normalized_ffmpeg_frame(play_base_frame + delta);
    return current_frame;
}

static uae_s64 ffmpeg_guess_duration_frames(void)
{
    if (!ffmpeg_format || ffmpeg_video_stream_index < 0) {
        return 0;
    }

    AVStream *stream = ffmpeg_format->streams[ffmpeg_video_stream_index];
    if (stream->nb_frames > 0) {
        return stream->nb_frames;
    }
    if (stream->duration != AV_NOPTS_VALUE && stream->duration > 0) {
        return av_rescale_q(stream->duration, stream->time_base,
            ffmpeg_frame_time_base());
    }
    if (ffmpeg_format->duration != AV_NOPTS_VALUE && ffmpeg_format->duration > 0) {
        AVRational av_time_base = { 1, AV_TIME_BASE };
        return av_rescale_q(ffmpeg_format->duration, av_time_base,
            ffmpeg_frame_time_base());
    }
    return 0;
}

static bool ffmpeg_open_decoder(int stream_index, enum AVMediaType type,
    AVCodecContext **ctx)
{
    if (!ffmpeg_format || stream_index < 0) {
        return false;
    }
    AVStream *stream = ffmpeg_format->streams[stream_index];
    if (stream->codecpar->codec_type != type) {
        return false;
    }

    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        write_log(_T("VIDEOGRAB: FFmpeg decoder not found for stream %d\n"),
            stream_index);
        return false;
    }

    *ctx = avcodec_alloc_context3(codec);
    if (!*ctx) {
        return false;
    }

    int err = avcodec_parameters_to_context(*ctx, stream->codecpar);
    if (err < 0) {
        char error[AV_ERROR_MAX_STRING_SIZE];
        write_log(_T("VIDEOGRAB: FFmpeg codec parameters failed: %s\n"),
            ffmpeg_error_text(err, error, sizeof error));
        avcodec_free_context(ctx);
        return false;
    }

    err = avcodec_open2(*ctx, codec, NULL);
    if (err < 0) {
        char error[AV_ERROR_MAX_STRING_SIZE];
        write_log(_T("VIDEOGRAB: FFmpeg codec open failed: %s\n"),
            ffmpeg_error_text(err, error, sizeof error));
        avcodec_free_context(ctx);
        return false;
    }
    return true;
}

#ifdef USE_SDL3
static void ffmpeg_close_audio_output(void)
{
    if (ffmpeg_audio_stream) {
        SDL_DestroyAudioStream(ffmpeg_audio_stream);
        ffmpeg_audio_stream = NULL;
    }
    if (ffmpeg_audio_sdl_initialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        ffmpeg_audio_sdl_initialized = false;
    }
}

static bool ffmpeg_open_audio_output(void)
{
    if (!ffmpeg_audio_codec || ffmpeg_audio_codec->sample_rate <= 0 ||
        ffmpeg_audio_codec->ch_layout.nb_channels <= 0) {
        return false;
    }

    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            write_log(_T("VIDEOGRAB: SDL audio init failed: %s\n"),
                SDL_GetError());
            return false;
        }
        ffmpeg_audio_sdl_initialized = true;
    }

    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 2);
    if (swr_alloc_set_opts2(&ffmpeg_swr,
            &out_layout,
            AV_SAMPLE_FMT_S16,
            ffmpeg_audio_codec->sample_rate,
            &ffmpeg_audio_codec->ch_layout,
            ffmpeg_audio_codec->sample_fmt,
            ffmpeg_audio_codec->sample_rate,
            0,
            NULL) < 0 || !ffmpeg_swr || swr_init(ffmpeg_swr) < 0) {
        av_channel_layout_uninit(&out_layout);
        write_log(_T("VIDEOGRAB: FFmpeg audio resampler init failed\n"));
        return false;
    }
    av_channel_layout_uninit(&out_layout);

    SDL_AudioSpec spec;
    std::memset(&spec, 0, sizeof spec);
    spec.freq = ffmpeg_audio_codec->sample_rate;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    ffmpeg_audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!ffmpeg_audio_stream) {
        write_log(_T("VIDEOGRAB: SDL audio stream open failed: %s\n"),
            SDL_GetError());
        return false;
    }

    ffmpeg_update_audio_gain();
    SDL_ResumeAudioStreamDevice(ffmpeg_audio_stream);
    write_log(_T("VIDEOGRAB: FFmpeg audio initialized, %d Hz stereo\n"),
        spec.freq);
    return true;
}

static void ffmpeg_filter_audio_channels(uae_s16 *samples, int frames)
{
    if (!samples || frames <= 0) {
        return;
    }
    if (audio_chflags == 1) {
        for (int i = 0; i < frames; i++) {
            samples[i * 2 + 1] = 0;
        }
    } else if (audio_chflags == 2) {
        for (int i = 0; i < frames; i++) {
            samples[i * 2] = 0;
        }
    }
}

static void ffmpeg_queue_audio_frame(AVFrame *frame)
{
    if (!ffmpeg_audio_stream || !ffmpeg_swr || !frame || frame->nb_samples <= 0) {
        return;
    }

    const int out_samples = swr_get_out_samples(ffmpeg_swr, frame->nb_samples);
    if (out_samples <= 0) {
        return;
    }

    std::vector<uae_u8> audio((size_t)out_samples * 2 * sizeof(uae_s16));
    uae_u8 *out_planes[] = { audio.data() };
    const int converted = swr_convert(ffmpeg_swr, out_planes, out_samples,
        (const uae_u8 **)frame->extended_data, frame->nb_samples);
    if (converted <= 0) {
        return;
    }

    ffmpeg_filter_audio_channels((uae_s16 *)audio.data(), converted);
    const int bytes = converted * 2 * (int)sizeof(uae_s16);
    if (!SDL_PutAudioStreamData(ffmpeg_audio_stream, audio.data(), bytes)) {
        write_log(_T("VIDEOGRAB: SDL_PutAudioStreamData failed: %s\n"),
            SDL_GetError());
    }
}

static void ffmpeg_clear_audio(void)
{
    if (ffmpeg_audio_stream) {
        SDL_ClearAudioStream(ffmpeg_audio_stream);
    }
    if (ffmpeg_swr) {
        swr_close(ffmpeg_swr);
        swr_init(ffmpeg_swr);
    }
}
#else
static void ffmpeg_close_audio_output(void)
{
}

static bool ffmpeg_open_audio_output(void)
{
    return false;
}

static void ffmpeg_queue_audio_frame(AVFrame *)
{
}

static void ffmpeg_clear_audio(void)
{
}
#endif

static void ffmpeg_decode_audio_packet(AVPacket *packet)
{
    if (!ffmpeg_audio_codec || !ffmpeg_audio_frame) {
        return;
    }
    int err = avcodec_send_packet(ffmpeg_audio_codec, packet);
    if (err < 0 && err != AVERROR(EAGAIN) && err != AVERROR_EOF) {
        return;
    }

    for (;;) {
        err = avcodec_receive_frame(ffmpeg_audio_codec, ffmpeg_audio_frame);
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            break;
        }
        if (err < 0) {
            break;
        }
        ffmpeg_queue_audio_frame(ffmpeg_audio_frame);
        av_frame_unref(ffmpeg_audio_frame);
    }
}

static bool ffmpeg_copy_video_frame(AVFrame *frame, uae_s64 frame_index)
{
    if (!frame || !ffmpeg_video_codec) {
        return false;
    }

    const int width = ffmpeg_video_codec->width;
    const int height = ffmpeg_video_codec->height;
    if (width <= 0 || height <= 0) {
        return false;
    }

    const size_t row_bytes = (size_t)width * 3;
    if (!resize_frame_buffer(row_bytes * (size_t)height)) {
        return false;
    }

    std::vector<uae_u8> top_down(row_bytes * (size_t)height);
    uae_u8 *dst_data[] = { top_down.data() };
    int dst_linesize[] = { (int)row_bytes };

    ffmpeg_sws = sws_getCachedContext(ffmpeg_sws,
        width,
        height,
        ffmpeg_video_codec->pix_fmt,
        width,
        height,
        AV_PIX_FMT_BGR24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL);
    if (!ffmpeg_sws) {
        return false;
    }

    sws_scale(ffmpeg_sws, frame->data, frame->linesize, 0, height,
        dst_data, dst_linesize);

    uae_u8 *dst = frame_bytes();
    for (int y = 0; y < height; y++) {
        const uae_u8 *src = top_down.data() + (size_t)y * row_bytes;
        std::memcpy(dst + (size_t)(height - 1 - y) * row_bytes, src,
            row_bytes);
    }

    video_width = width;
    video_height = height;
    loaded_frame = frame_index;
    ffmpeg_decoded_frame = frame_index;
    return true;
}

static bool ffmpeg_decode_video_packet(AVPacket *packet, uae_s64 target_frame,
    bool *packet_consumed)
{
    if (packet_consumed) {
        *packet_consumed = false;
    }
    int err = avcodec_send_packet(ffmpeg_video_codec, packet);
    if (err != AVERROR(EAGAIN) && packet_consumed) {
        *packet_consumed = true;
    }
    if (err < 0 && err != AVERROR(EAGAIN) && err != AVERROR_EOF) {
        return false;
    }

    for (;;) {
        err = avcodec_receive_frame(ffmpeg_video_codec, ffmpeg_video_frame);
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            break;
        }
        if (err < 0) {
            if (packet_consumed) {
                *packet_consumed = true;
            }
            return false;
        }

        const uae_s64 frame_index =
            ffmpeg_frame_from_pts(ffmpeg_video_frame->best_effort_timestamp);
        if (frame_index >= target_frame) {
            const bool copied = ffmpeg_copy_video_frame(ffmpeg_video_frame,
                frame_index);
            av_frame_unref(ffmpeg_video_frame);
            return copied;
        }
        ffmpeg_decoded_frame = frame_index;
        av_frame_unref(ffmpeg_video_frame);
    }
    return false;
}

static bool ffmpeg_seek_frame(uae_s64 frame, bool clear_audio_output)
{
    if (!ffmpeg_format || ffmpeg_video_stream_index < 0) {
        return false;
    }

    frame = normalized_ffmpeg_frame(frame);
    const uae_s64 timestamp = ffmpeg_timestamp_from_frame(frame);
    int err = av_seek_frame(ffmpeg_format, ffmpeg_video_stream_index,
        timestamp, AVSEEK_FLAG_BACKWARD);
    if (err < 0) {
        char error[AV_ERROR_MAX_STRING_SIZE];
        write_log(_T("VIDEOGRAB: FFmpeg seek to frame %lld failed: %s\n"),
            frame, ffmpeg_error_text(err, error, sizeof error));
        return false;
    }
    avcodec_flush_buffers(ffmpeg_video_codec);
    if (ffmpeg_audio_codec) {
        avcodec_flush_buffers(ffmpeg_audio_codec);
    }
    ffmpeg_audio_discard_until_frame =
        ffmpeg_audio_stream_index >= 0 && frame > 0 ? frame : -1;
    ffmpeg_decoded_frame = -1;
    loaded_frame = -1;
    if (clear_audio_output) {
        ffmpeg_clear_audio();
    }
    if (ffmpeg_packet) {
        av_packet_unref(ffmpeg_packet);
    }
    ffmpeg_packet_pending = false;
    return true;
}

static bool read_ffmpeg_frame(uae_s64 target_frame)
{
    if (!ffmpeg_format || !ffmpeg_video_codec || !ffmpeg_packet) {
        return false;
    }

    target_frame = normalized_ffmpeg_frame(target_frame);
    if (loaded_frame == target_frame && !frame_buffer.empty()) {
        return true;
    }

    if (ffmpeg_decoded_frame < 0 || target_frame < ffmpeg_decoded_frame ||
        target_frame > ffmpeg_decoded_frame + 120) {
        if (!ffmpeg_seek_frame(target_frame, true)) {
            return false;
        }
    }

    bool looped = false;
    for (;;) {
        if (!ffmpeg_packet_pending) {
            int err = av_read_frame(ffmpeg_format, ffmpeg_packet);
            if (err == AVERROR_EOF) {
                ffmpeg_decode_audio_packet(nullptr);
                if (ffmpeg_decode_video_packet(nullptr, target_frame, nullptr)) {
                    return true;
                }
                if (looped) {
                    return !frame_buffer.empty();
                }
                looped = true;
                if (!ffmpeg_seek_frame(0, false)) {
                    return false;
                }
                current_frame = 0;
                play_base_frame = 0;
                play_base_time = std::chrono::steady_clock::now();
                target_frame = 0;
                continue;
            }
            if (err < 0) {
                char error[AV_ERROR_MAX_STRING_SIZE];
                write_log(_T("VIDEOGRAB: FFmpeg read failed: %s\n"),
                    ffmpeg_error_text(err, error, sizeof error));
                return !frame_buffer.empty();
            }
        }

        bool got_frame = false;
        bool packet_consumed = true;
        if (ffmpeg_packet->stream_index == ffmpeg_video_stream_index) {
            got_frame = ffmpeg_decode_video_packet(ffmpeg_packet, target_frame,
                &packet_consumed);
        } else if (ffmpeg_packet->stream_index == ffmpeg_audio_stream_index) {
            if (!ffmpeg_discard_audio_packet(ffmpeg_packet)) {
                ffmpeg_decode_audio_packet(ffmpeg_packet);
            }
        }
        if (packet_consumed) {
            av_packet_unref(ffmpeg_packet);
            ffmpeg_packet_pending = false;
        } else {
            ffmpeg_packet_pending = true;
        }
        if (got_frame) {
            return true;
        }
    }
}

static void uninit_ffmpeg_videograb(void)
{
    ffmpeg_close_audio_output();
    if (ffmpeg_swr) {
        swr_free(&ffmpeg_swr);
    }
    if (ffmpeg_sws) {
        sws_freeContext(ffmpeg_sws);
        ffmpeg_sws = NULL;
    }
    if (ffmpeg_packet) {
        av_packet_free(&ffmpeg_packet);
    }
    if (ffmpeg_video_frame) {
        av_frame_free(&ffmpeg_video_frame);
    }
    if (ffmpeg_audio_frame) {
        av_frame_free(&ffmpeg_audio_frame);
    }
    if (ffmpeg_video_codec) {
        avcodec_free_context(&ffmpeg_video_codec);
    }
    if (ffmpeg_audio_codec) {
        avcodec_free_context(&ffmpeg_audio_codec);
    }
    if (ffmpeg_format) {
        avformat_close_input(&ffmpeg_format);
    }
    ffmpeg_video_stream_index = -1;
    ffmpeg_audio_stream_index = -1;
    ffmpeg_duration_frames = 0;
    ffmpeg_decoded_frame = -1;
    ffmpeg_audio_discard_until_frame = -1;
    ffmpeg_packet_pending = false;
}

static bool init_ffmpeg_videograb(const TCHAR *filename)
{
    int err = avformat_open_input(&ffmpeg_format, filename, NULL, NULL);
    if (err < 0) {
        return false;
    }

    err = avformat_find_stream_info(ffmpeg_format, NULL);
    if (err < 0) {
        char error[AV_ERROR_MAX_STRING_SIZE];
        write_log(_T("VIDEOGRAB: FFmpeg stream info failed for '%s': %s\n"),
            filename, ffmpeg_error_text(err, error, sizeof error));
        uninit_ffmpeg_videograb();
        return false;
    }

    ffmpeg_video_stream_index = av_find_best_stream(ffmpeg_format,
        AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ffmpeg_video_stream_index < 0 ||
        !ffmpeg_open_decoder(ffmpeg_video_stream_index, AVMEDIA_TYPE_VIDEO,
            &ffmpeg_video_codec)) {
        uninit_ffmpeg_videograb();
        return false;
    }

    const int audio_index = av_find_best_stream(ffmpeg_format,
        AVMEDIA_TYPE_AUDIO, -1, ffmpeg_video_stream_index, NULL, 0);
    if (audio_index >= 0 &&
        ffmpeg_open_decoder(audio_index, AVMEDIA_TYPE_AUDIO,
            &ffmpeg_audio_codec)) {
        ffmpeg_audio_frame = av_frame_alloc();
        if (ffmpeg_audio_frame && ffmpeg_open_audio_output()) {
            ffmpeg_audio_stream_index = audio_index;
        } else {
            av_frame_free(&ffmpeg_audio_frame);
            avcodec_free_context(&ffmpeg_audio_codec);
        }
    }

    ffmpeg_video_frame = av_frame_alloc();
    ffmpeg_packet = av_packet_alloc();
    if (!ffmpeg_video_frame || !ffmpeg_packet) {
        uninit_ffmpeg_videograb();
        return false;
    }

    AVStream *stream = ffmpeg_format->streams[ffmpeg_video_stream_index];
    ffmpeg_frame_rate = av_guess_frame_rate(ffmpeg_format, stream, NULL);
    if (ffmpeg_frame_rate.num <= 0 || ffmpeg_frame_rate.den <= 0) {
        ffmpeg_frame_rate.num = 25;
        ffmpeg_frame_rate.den = 1;
    }
    ffmpeg_duration_frames = ffmpeg_guess_duration_frames();
    AVRational microsecond_base = { 1, 1000000 };
    frame_usec = (uae_u32)std::max<uae_s64>(1,
        av_rescale_q(1, ffmpeg_frame_time_base(), microsecond_base));
    video_width = ffmpeg_video_codec->width;
    video_height = ffmpeg_video_codec->height;
    current_frame = 0;
    play_base_frame = 0;
    play_base_time = std::chrono::steady_clock::now();
    video_paused = 0;
    loaded_frame = -1;
    ffmpeg_decoded_frame = -1;

    write_log(_T("VIDEOGRAB: FFmpeg playing '%s', %dx%d, %.3f fps%s\n"),
        filename, video_width, video_height,
        (double)ffmpeg_frame_rate.num / (double)ffmpeg_frame_rate.den,
        ffmpeg_audio_codec ? _T(", audio") : _T(""));
    return true;
}
#endif

#ifdef USE_SDL3
static bool init_camera_videograb(void)
{
    if (!(SDL_WasInit(SDL_INIT_CAMERA) & SDL_INIT_CAMERA)) {
        if (!SDL_InitSubSystem(SDL_INIT_CAMERA)) {
            write_log(_T("VIDEOGRAB: SDL camera init failed: %s\n"), SDL_GetError());
            return false;
        }
        camera_sdl_initialized = true;
    }

    int count = 0;
    SDL_CameraID *cameras = SDL_GetCameras(&count);
    if (!cameras || count <= 0) {
        write_log(_T("VIDEOGRAB: no SDL camera devices found: %s\n"), SDL_GetError());
        if (cameras) {
            SDL_free(cameras);
        }
        return false;
    }

    const SDL_CameraID camera_id = cameras[0];
    const char *camera_name = SDL_GetCameraName(camera_id);
    camera = SDL_OpenCamera(camera_id, NULL);
    SDL_free(cameras);
    if (!camera) {
        write_log(_T("VIDEOGRAB: SDL_OpenCamera failed: %s\n"), SDL_GetError());
        return false;
    }

    video_width = 0;
    video_height = 0;
    video_paused = 0;
    current_frame = 0;
    loaded_frame = -1;
    camera_permission_denied_logged = false;
    write_log(_T("VIDEOGRAB: opened camera '%s'\n"),
        camera_name ? camera_name : _T("<unknown>"));
    return true;
}

static bool copy_camera_surface(SDL_Surface *surface)
{
    if (!surface || surface->w <= 0 || surface->h <= 0 || !surface->pixels) {
        return false;
    }

    SDL_Surface *converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_BGR24);
    if (!converted) {
        write_log(_T("VIDEOGRAB: SDL_ConvertSurface failed: %s\n"), SDL_GetError());
        return false;
    }

    const size_t row_bytes = (size_t)converted->w * 3;
    if (!resize_frame_buffer(row_bytes * (size_t)converted->h)) {
        SDL_DestroySurface(converted);
        return false;
    }

    const uae_u8 *src_pixels = (const uae_u8 *)converted->pixels;
    uae_u8 *dst_pixels = frame_bytes();
    for (int y = 0; y < converted->h; y++) {
        const uae_u8 *src = src_pixels + (size_t)y * converted->pitch;
        uae_u8 *dst = dst_pixels + (size_t)(converted->h - 1 - y) * row_bytes;
        std::memcpy(dst, src, row_bytes);
    }

    video_width = converted->w;
    video_height = converted->h;
    SDL_DestroySurface(converted);
    return true;
}

static bool get_camera_videograb(long **buffer, int *width, int *height)
{
    if (!camera) {
        return false;
    }
    if (video_paused > 0 && !frame_buffer.empty()) {
        *buffer = frame_buffer.data();
        *width = video_width;
        *height = video_height;
        return true;
    }

    // SDL 3.2 returned the permission state as an int; SDL 3.4 gave the
    // same -1/0/1 values a named enum. Keep compatibility with both APIs.
    const int permission = SDL_GetCameraPermissionState(camera);
    if (permission < 0) {
        if (!camera_permission_denied_logged) {
            write_log(_T("VIDEOGRAB: camera permission denied\n"));
            camera_permission_denied_logged = true;
        }
        return false;
    }

    Uint64 timestamp = 0;
    SDL_Surface *surface = SDL_AcquireCameraFrame(camera, &timestamp);
    if (surface) {
        const bool copied = copy_camera_surface(surface);
        SDL_ReleaseCameraFrame(camera, surface);
        if (!copied) {
            return false;
        }
        current_frame++;
    }

    if (frame_buffer.empty()) {
        return false;
    }

    *buffer = frame_buffer.data();
    *width = video_width;
    *height = video_height;
    return true;
}
#else
static bool init_camera_videograb(void)
{
    write_log(_T("VIDEOGRAB: capture device support requires SDL3\n"));
    return false;
}

static bool get_camera_videograb(long **, int *, int *)
{
    return false;
}
#endif

void uninitvideograb(void)
{
#ifdef AMIBERRY_WITH_FFMPEG
    uninit_ffmpeg_videograb();
#endif
#ifdef USE_SDL3
    if (camera) {
        SDL_CloseCamera(camera);
        camera = NULL;
    }
    if (camera_sdl_initialized) {
        SDL_QuitSubSystem(SDL_INIT_CAMERA);
        camera_sdl_initialized = false;
    }
#endif
    if (avi_file) {
        std::fclose(avi_file);
        avi_file = NULL;
    }
    avi_frames.clear();
    frame_buffer.clear();
    videograb_mode = VIDEOGRAB_NONE;
    video_initialized = false;
    video_paused = -1;
    video_width = 0;
    video_height = 0;
    current_frame = 0;
    play_base_frame = 0;
    frame_usec = 40000;
    loaded_frame = -1;
    audio_chflags = 0;
    audio_volume = 0;
    audio_muted = false;
}

bool initvideograb(const TCHAR *filename)
{
    uninitvideograb();

    if (!filename || !filename[0]) {
        if (!init_camera_videograb()) {
            uninitvideograb();
            return false;
        }
        videograb_mode = VIDEOGRAB_CAMERA;
    } else {
#ifdef AMIBERRY_WITH_FFMPEG
        if (init_ffmpeg_videograb(filename)) {
            videograb_mode = VIDEOGRAB_FFMPEG;
        } else
#endif
        if (!init_avi_videograb(filename)) {
            uninitvideograb();
            return false;
        } else {
            videograb_mode = VIDEOGRAB_AVI;
        }
    }

    if (videograb_mode == VIDEOGRAB_FFMPEG) {
        audio_volume = 100 - currprefs.sound_volume_genlock;
#ifdef AMIBERRY_WITH_FFMPEG
        audio_master_volume = 100 - currprefs.sound_volume_master;
#endif
        if (currprefs.genlock_image == 4) {
            audio_chflags = 3;
        }
#ifdef AMIBERRY_WITH_FFMPEG
        ffmpeg_update_audio_gain();
#endif
    }

    video_initialized = true;
    return true;
}

bool getvideograb(long **buffer, int *width, int *height)
{
    if (!video_initialized || !buffer || !width || !height) {
        return false;
    }

    if (videograb_mode == VIDEOGRAB_CAMERA) {
        return get_camera_videograb(buffer, width, height);
    }

    if (videograb_mode == VIDEOGRAB_FFMPEG) {
#ifdef AMIBERRY_WITH_FFMPEG
        const uae_s64 frame = ffmpeg_current_frame();
        if (!read_ffmpeg_frame(frame)) {
            return false;
        }
        *buffer = frame_buffer.data();
        *width = video_width;
        *height = video_height;
        return true;
#else
        return false;
#endif
    }

    if (videograb_mode != VIDEOGRAB_AVI) {
        return false;
    }

    const uae_s64 frame = current_avi_frame();
    if (!read_avi_frame(frame)) {
        return false;
    }

    *buffer = frame_buffer.data();
    *width = video_width;
    *height = video_height;
    return true;
}

void pausevideograb(int pause)
{
    if (!video_initialized) {
        return;
    }
    if (pause < 0) {
        pause = video_paused > 0 ? 0 : 1;
    }
    if (video_paused == pause) {
        return;
    }

    if (videograb_mode == VIDEOGRAB_AVI || videograb_mode == VIDEOGRAB_FFMPEG) {
        if (pause > 0) {
            if (videograb_mode == VIDEOGRAB_AVI) {
                current_frame = current_avi_frame();
            }
#ifdef AMIBERRY_WITH_FFMPEG
            else if (videograb_mode == VIDEOGRAB_FFMPEG) {
                current_frame = ffmpeg_current_frame();
            }
#endif
        } else {
            play_base_frame = current_frame;
            play_base_time = std::chrono::steady_clock::now();
        }
    }
#if defined(AMIBERRY_WITH_FFMPEG) && defined(USE_SDL3)
    if (videograb_mode == VIDEOGRAB_FFMPEG && ffmpeg_audio_stream) {
        if (pause > 0) {
            SDL_PauseAudioStreamDevice(ffmpeg_audio_stream);
        } else {
            SDL_ResumeAudioStreamDevice(ffmpeg_audio_stream);
        }
    }
#endif
    video_paused = pause > 0 ? 1 : 0;
}

uae_s64 getsetpositionvideograb(uae_s64 framepos)
{
    if (!video_initialized) {
        return 0;
    }

    if (framepos < 0) {
        if (videograb_mode == VIDEOGRAB_AVI) {
            return current_avi_frame();
        }
#ifdef AMIBERRY_WITH_FFMPEG
        if (videograb_mode == VIDEOGRAB_FFMPEG) {
            return ffmpeg_current_frame();
        }
#endif
        return current_frame;
    }

    if (videograb_mode == VIDEOGRAB_AVI) {
        current_frame = normalized_frame(framepos);
        play_base_frame = current_frame;
        play_base_time = std::chrono::steady_clock::now();
        loaded_frame = -1;
        return current_frame;
    }
#ifdef AMIBERRY_WITH_FFMPEG
    if (videograb_mode == VIDEOGRAB_FFMPEG) {
        current_frame = normalized_ffmpeg_frame(framepos);
        play_base_frame = current_frame;
        play_base_time = std::chrono::steady_clock::now();
        ffmpeg_seek_frame(current_frame, true);
        return current_frame;
    }
#endif

    current_frame = framepos;
    return current_frame;
}

uae_s64 getdurationvideograb(void)
{
    if (!video_initialized) {
        return 0;
    }
    if (videograb_mode == VIDEOGRAB_AVI) {
        return (uae_s64)avi_frames.size();
    }
#ifdef AMIBERRY_WITH_FFMPEG
    if (videograb_mode == VIDEOGRAB_FFMPEG) {
        return ffmpeg_duration_frames;
    }
#endif
    return 0;
}

bool isvideograb(void)
{
    return video_initialized;
}

bool getpausevideograb(void)
{
    return video_paused > 0;
}

void setvolumevideograb(int volume)
{
    audio_volume = volume;
#ifdef AMIBERRY_WITH_FFMPEG
    ffmpeg_update_audio_gain();
#endif
}

void setmastervolumevideograb(int volume, bool mute)
{
#ifdef AMIBERRY_WITH_FFMPEG
    audio_master_volume = 100 - std::max(0, std::min(100, volume));
    audio_master_muted = mute;
    ffmpeg_update_audio_gain();
#else
    (void)volume;
    (void)mute;
#endif
}

void setchflagsvideograb(int chflags, bool mute)
{
    audio_chflags = chflags;
    audio_muted = mute;
#ifdef AMIBERRY_WITH_FFMPEG
    ffmpeg_update_audio_gain();
#endif
}

void isvideograb_status(void)
{
    if (!video_initialized) {
        return;
    }
    if (currprefs.sound_volume_genlock != changed_prefs.sound_volume_genlock) {
        currprefs.sound_volume_genlock = changed_prefs.sound_volume_genlock;
        setvolumevideograb(100 - currprefs.sound_volume_genlock);
        setchflagsvideograb(audio_chflags, audio_muted);
    }
}
