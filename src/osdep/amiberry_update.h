#pragma once

#include <string>
#include <functional>
#include <mutex>
#include <atomic>

// Update channel
enum class UpdateChannel {
	Stable,
	Preview
};

// How this platform handles updates
enum class UpdateMethod {
	SELF_UPDATE,    // Can download and replace binary
	NOTIFY_ONLY,   // Can check, but user must update manually
	DISABLED        // No update checking (Flatpak, Android, LibRetro)
};

// Result of a version check
struct UpdateInfo {
	bool update_available = false;
	std::string latest_version;      // e.g. "8.1.0" or "8.0.0-pre.32"
	std::string current_version;     // e.g. "8.0.0-pre.30"
	std::string release_url;         // GitHub release page URL
	std::string download_url;        // Direct download URL for this platform's asset
	std::string release_notes;       // Release body (markdown)
	std::string tag_name;            // e.g. "v8.1.0" or "preview-v8.0.0-pre.32"
	std::string asset_name;          // e.g. "amiberry-8.1.0-linux-x86_64.zip"
	int64_t asset_size = 0;          // Download size in bytes
	std::string sha256_expected;     // Expected SHA256 hash (from SHA256SUMS asset)
	bool is_prerelease = false;
	std::string error_message;       // Non-empty if check failed
};

// Progress callback: (bytes_downloaded, total_bytes) -> should_cancel
using DownloadProgressCallback = std::function<bool(int64_t, int64_t)>;

// Determine how this platform handles updates
UpdateMethod get_update_method();

// Get current version string for comparison (e.g. "8.0.0" or "8.0.0-pre.30")
std::string get_current_semver();

// Compare two semver strings. Returns:
//  -1 if a < b, 0 if a == b, 1 if a > b
// Handles prerelease: 8.0.0-pre.30 < 8.0.0-pre.31 < 8.0.0
int compare_versions(const std::string& a, const std::string& b);

// Check for updates (blocking, call from background thread)
UpdateInfo check_for_updates(UpdateChannel channel);

// Download the update asset to a temporary file (blocking, with progress)
// Returns path to downloaded file, or empty string on failure
std::string download_update(const UpdateInfo& info, DownloadProgressCallback progress_cb = nullptr);

// Verify SHA256 checksum of downloaded file
bool verify_update_checksum(const std::string& file_path, const std::string& expected_sha256);

// Apply the update (platform-specific: extract, replace binary, prepare restart)
// Returns true if ready to restart
bool apply_update(const std::string& downloaded_file, const UpdateInfo& info);

// Launch the updated binary and exit current process
// This function does not return on success
void restart_after_update();

// --- Async update check (called from main thread) ---

// Start an async update check. Results retrieved via get_async_update_result()
void start_async_update_check(UpdateChannel channel);

// Check if async update check is still running
bool is_update_check_running();

// Get the result of the async update check (only valid after is_update_check_running() returns false)
UpdateInfo get_async_update_result();

// Cancel any running async update check
void cancel_async_update_check();
