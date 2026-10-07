#include "sysconfig.h"
#include "sysdeps.h"

#include "dmabuf_surface.h"

#if defined(__linux__) && !defined(__ANDROID__) && !defined(LIBRETRO)
#include <atomic>
#include <mutex>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace {
constexpr const char* k_property = "amiberry.dmabuf_surface";

constexpr uint32_t fourcc(char a, char b, char c, char d)
{
	return uint32_t(uint8_t(a)) | uint32_t(uint8_t(b)) << 8 | uint32_t(uint8_t(c)) << 16 | uint32_t(uint8_t(d)) << 24;
}

// Alpha is never meaningful in the frame, so import as the X variants.
uint32_t drm_fourcc_for(SDL_PixelFormat format)
{
	switch (format) {
	case SDL_PIXELFORMAT_ABGR8888:
	case SDL_PIXELFORMAT_XBGR8888:
		return fourcc('X', 'B', '2', '4');
	case SDL_PIXELFORMAT_ARGB8888:
	case SDL_PIXELFORMAT_XRGB8888:
		return fourcc('X', 'R', '2', '4');
	default:
		return 0;
	}
}

int heap_fd()
{
	static int fd = -2;
	if (fd == -2) {
		fd = open("/dev/dma_heap/system", O_RDWR | O_CLOEXEC);
		if (fd < 0)
			write_log("dmabuf surface: /dev/dma_heap/system unavailable (%s); frames use texture uploads\n",
				strerror(errno));
	}
	return fd;
}

void release(void* /*userdata*/, void* value)
{
	auto* info = static_cast<DmabufSurfaceInfo*>(value);
	munmap(info->map, info->size);
	close(info->fd);
	delete info;
}

// Only EINTR is retried: any other failure (including EAGAIN) is reported so
// the renderer can fall back to uploads instead of spinning on the frame path.
bool sync(const int fd, const uint64_t flags)
{
	dma_buf_sync arg{};
	arg.flags = flags;
	while (ioctl(fd, DMA_BUF_IOCTL_SYNC, &arg) < 0) {
		if (errno != EINTR)
			return false;
	}
	return true;
}

// Duplicate of the dma-buf the GPU is reading, or -1. A duplicate keeps the
// buffer valid even if its surface is destroyed before the reclaim. It stays
// set until the reclaim has finished waiting, so a concurrent writer never
// mistakes an in-progress reclaim for a completed one.
std::atomic<int> g_pending_reclaim_fd{-1};

// Serializes every consumer of g_pending_reclaim_fd (Denise thread and main
// thread writers, and the next present).
std::mutex g_reclaim_mutex;

// Set once the kernel refuses a CPU-access sync: the ownership contract can no
// longer be guaranteed, so no further frame is handed to the GPU.
std::atomic<bool> g_sync_failed{false};

// Give the CPU write access after a GPU read, waiting for that read to finish.
// If the sync ioctl fails, wait on the buffer's fences with poll() instead
// and stop using dma-buf frames. POLLOUT signals once every fence has
// completed; nothing returns before it is seen, the same blocking contract as
// the ioctl. Other poll() failures on a held fd are transient (EINTR, ENOMEM),
// so they back off and retry.
void reclaim_fd(const int fd)
{
	if (sync(fd, DMA_BUF_SYNC_START | DMA_BUF_SYNC_WRITE))
		return;
	const int err = errno;
	if (!g_sync_failed.exchange(true))
		write_log("dmabuf surface: CPU access sync failed (%s); frames use texture uploads\n", strerror(err));
	for (;;) {
		pollfd p{ fd, POLLOUT, 0 };
		const int ready = poll(&p, 1, -1);
		if (ready > 0 && (p.revents & POLLOUT))
			return;
		if (ready < 0 && errno == EINTR)
			continue;
		SDL_Delay(1);
	}
}

// Caller holds g_reclaim_mutex.
void reclaim_pending_locked()
{
	const int fd = g_pending_reclaim_fd.load(std::memory_order_relaxed);
	if (fd < 0)
		return;
	reclaim_fd(fd);
	close(fd);
	g_pending_reclaim_fd.store(-1, std::memory_order_release);
}
}

SDL_Surface* dmabuf_surface_create(int width, int height, SDL_PixelFormat format)
{
	const uint32_t drm_fourcc = drm_fourcc_for(format);
	if (g_sync_failed.load(std::memory_order_relaxed))
		return nullptr;
	if (width <= 0 || height <= 0 || drm_fourcc == 0)
		return nullptr;
	const int heap = heap_fd();
	if (heap < 0)
		return nullptr;

	// 64-byte row alignment satisfies the GPU import pitch rules we target and
	// keeps every row cache-line aligned for the CPU writer.
	const int pitch = (width * 4 + 63) & ~63;
	const size_t size = size_t(pitch) * size_t(height);
	dma_heap_allocation_data alloc{};
	alloc.len = size;
	alloc.fd_flags = O_RDWR | O_CLOEXEC;
	if (ioctl(heap, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
		write_log("dmabuf surface: allocation of %zu bytes failed (%s)\n", size, strerror(errno));
		return nullptr;
	}
	const int fd = int(alloc.fd);
	void* map = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		write_log("dmabuf surface: mmap failed (%s)\n", strerror(errno));
		close(fd);
		return nullptr;
	}

	SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, format, map, pitch);
	if (!surface) {
		munmap(map, size);
		close(fd);
		return nullptr;
	}

	static std::atomic<uint64_t> next_generation{1};
	auto* info = new DmabufSurfaceInfo;
	info->fd = fd;
	info->width = width;
	info->height = height;
	info->pitch = pitch;
	info->drm_fourcc = drm_fourcc;
	info->map = map;
	info->size = size;
	info->generation = next_generation.fetch_add(1);
	if (!SDL_SetPointerPropertyWithCleanup(SDL_GetSurfaceProperties(surface), k_property, info, release, nullptr)) {
		// SDL already ran the cleanup on failure.
		SDL_DestroySurface(surface);
		return nullptr;
	}
	if (!sync(fd, DMA_BUF_SYNC_START | DMA_BUF_SYNC_WRITE)) {
		write_log("dmabuf surface: DMA_BUF_IOCTL_SYNC unsupported (%s); frames use texture uploads\n", strerror(errno));
		SDL_DestroySurface(surface);
		return nullptr;
	}
	return surface;
}

const DmabufSurfaceInfo* dmabuf_surface_info(SDL_Surface* surface)
{
	if (!surface)
		return nullptr;
	return static_cast<const DmabufSurfaceInfo*>(
		SDL_GetPointerProperty(SDL_GetSurfaceProperties(surface), k_property, nullptr));
}

bool dmabuf_surface_end_cpu_access(const DmabufSurfaceInfo* info)
{
	// Settle a reclaim nobody consumed (a frame without drawn lines) so
	// START/END stay paired.
	dmabuf_surface_reclaim();
	if (g_sync_failed.load(std::memory_order_relaxed))
		return false;
	return info && sync(info->fd, DMA_BUF_SYNC_END | DMA_BUF_SYNC_WRITE);
}

void dmabuf_surface_release_to_gpu(const DmabufSurfaceInfo* info)
{
	if (!info)
		return;
	const std::lock_guard<std::mutex> lock(g_reclaim_mutex);
	reclaim_pending_locked();
	const int fd = fcntl(info->fd, F_DUPFD_CLOEXEC, 0);
	if (fd < 0) {
		// No handle to defer with: reclaim now.
		reclaim_fd(info->fd);
		return;
	}
	g_pending_reclaim_fd.store(fd, std::memory_order_release);
}

void dmabuf_surface_reclaim()
{
	if (g_pending_reclaim_fd.load(std::memory_order_acquire) < 0)
		return;
	const std::lock_guard<std::mutex> lock(g_reclaim_mutex);
	reclaim_pending_locked();
}

#else

SDL_Surface* dmabuf_surface_create(int, int, SDL_PixelFormat)
{
	return nullptr;
}

const DmabufSurfaceInfo* dmabuf_surface_info(SDL_Surface*)
{
	return nullptr;
}

bool dmabuf_surface_end_cpu_access(const DmabufSurfaceInfo*)
{
	return false;
}

void dmabuf_surface_release_to_gpu(const DmabufSurfaceInfo*)
{
}

void dmabuf_surface_reclaim()
{
}

#endif
