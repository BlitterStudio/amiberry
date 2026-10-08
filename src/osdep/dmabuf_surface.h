#pragma once

// Native-chipset frame surfaces backed by a CPU-cached dma-buf (Linux
// dma-heap). A GPU renderer can import such a surface as an EGLImage and
// sample it directly instead of uploading the frame every vsync; on V3D that
// replaces ~1.5 ms/frame of CPU texture tiling with a GPU-side conversion.
//
// Ownership contract: the CPU owns the pixels except between
// dmabuf_surface_release_to_gpu() (inside the renderer's present) and the
// next dmabuf_surface_reclaim(), which every frame writer calls before
// touching pixels. Reclaim blocks until the GPU's implicit read fences
// signal, so the read overlaps emulation instead of stalling present. If the
// kernel ever refuses a sync, reclaim waits on the fences with poll() and
// end_cpu_access() fails from then on, so the renderer reverts to uploads.

#include <SDL3/SDL.h>
#include <cstddef>
#include <cstdint>

struct DmabufSurfaceInfo {
	int fd = -1;
	int width = 0;
	int height = 0;
	int pitch = 0;
	uint32_t drm_fourcc = 0;
	void* map = nullptr;
	size_t size = 0;
	// Unique per allocation so importers notice a replaced surface even when
	// the allocator hands back the same SDL_Surface address.
	uint64_t generation = 0;
};

// Returns nullptr when dma-buf backing is unavailable (non-Linux, no
// /dev/dma_heap/system access, unsupported format); callers then fall back to
// SDL_CreateSurface. The buffer is released when the surface is destroyed.
SDL_Surface* dmabuf_surface_create(int width, int height, SDL_PixelFormat format);

// nullptr for surfaces not created by dmabuf_surface_create().
const DmabufSurfaceInfo* dmabuf_surface_info(SDL_Surface* surface);

// Flush CPU writes so the GPU sees the complete frame. Call before
// submitting the GPU read. On false the CPU still owns the pixels and the
// caller must not let the GPU read them (fall back to uploads).
bool dmabuf_surface_end_cpu_access(const DmabufSurfaceInfo* info);

// Hand the surface to the GPU after submitting its read; the next
// dmabuf_surface_reclaim() waits for that read to finish.
void dmabuf_surface_release_to_gpu(const DmabufSurfaceInfo* info);

// Called by frame writers before writing pixels. Cheap when nothing is
// pending; otherwise waits for the GPU read released above.
void dmabuf_surface_reclaim();
