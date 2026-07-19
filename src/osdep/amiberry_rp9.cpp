#include "sysdeps.h"

#include "amiberry_rp9.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "archivers/zip/unzip.h"
#include "amiga_constants.h"
#include "blkdev.h"
#include "custom.h"
#include "cpuboard.h"
#include "disk.h"
#include "gfxboard.h"
#include "inputdevice.h"
#include "memory.h"
#include "newcpu.h"
#include "options.h"
#include "rommgr.h"
#include "rp9_manifest.h"
#include "savestate.h"
#include "target.h"
#include "zfile.h"

namespace
{
constexpr const char* manifest_name = "rp9-manifest.xml";
constexpr std::uint64_t maximum_manifest_size = 4 * 1024 * 1024;
constexpr std::uint64_t maximum_extracted_size = 16ULL * 1024 * 1024 * 1024;
constexpr std::uintmax_t maximum_scanned_rom_size = 10000000;
constexpr unsigned long maximum_archive_entries = 10000;
constexpr std::size_t maximum_entry_name = 4096;
constexpr std::size_t extraction_buffer_size = 128 * 1024;
constexpr int invalid_system_rom = -2;

std::string loaded_path;
std::string last_error;
std::string loaded_snapshot_path;
bool loaded_has_clip;
std::vector<std::filesystem::path> temporary_directories;
std::vector<std::string> loaded_floppy_paths;
std::vector<std::string> loaded_cd_paths;
std::atomic<unsigned long long> directory_sequence { 0 };

bool directory_contains_path(const std::filesystem::path& directory, const TCHAR* value)
{
	if (!value || !value[0])
		return false;
	const auto normalized_directory = directory.lexically_normal();
	const auto normalized_path = std::filesystem::path(value).lexically_normal();
	auto directory_component = normalized_directory.begin();
	auto path_component = normalized_path.begin();
	while (directory_component != normalized_directory.end() && path_component != normalized_path.end()) {
		if (*directory_component != *path_component)
			return false;
		++directory_component;
		++path_component;
	}
	return directory_component == normalized_directory.end();
}

bool prefs_reference_directory(const uae_prefs* prefs, const std::filesystem::path& directory)
{
	if (!prefs)
		return false;
	for (const auto& slot : prefs->floppyslots) {
		if (directory_contains_path(directory, slot.df))
			return true;
	}
	for (const auto& path : prefs->dfxlist) {
		if (directory_contains_path(directory, path))
			return true;
	}
	for (const auto& slot : prefs->cdslots) {
		if (slot.inuse && directory_contains_path(directory, slot.name))
			return true;
	}
	const auto mount_count = std::clamp(prefs->mountitems, 0, MOUNT_CONFIG_SIZE);
	for (int index = 0; index < mount_count; ++index) {
		if (directory_contains_path(directory, prefs->mountconfig[index].ci.rootdir))
			return true;
	}
	return false;
}

bool pending_snapshot_references_directory(const std::filesystem::path& directory)
{
	return savestate_state != 0 && directory_contains_path(directory, savestate_fname);
}

bool loaded_cd_paths_reference_directory(const std::filesystem::path& directory)
{
	return std::any_of(loaded_cd_paths.begin(), loaded_cd_paths.end(), [&directory](const auto& path) {
		return directory_contains_path(directory, path.c_str());
	});
}

bool loaded_floppy_paths_reference_directory(const std::filesystem::path& directory)
{
	return std::any_of(loaded_floppy_paths.begin(), loaded_floppy_paths.end(), [&directory](const auto& path) {
		return directory_contains_path(directory, path.c_str());
	});
}

void cleanup_unused_temporary_directories(const uae_prefs* additional_prefs = nullptr)
{
	for (auto directory = temporary_directories.begin(); directory != temporary_directories.end();) {
		if (prefs_reference_directory(&currprefs, *directory)
			|| prefs_reference_directory(&changed_prefs, *directory)
			|| (additional_prefs != &currprefs && additional_prefs != &changed_prefs
				&& prefs_reference_directory(additional_prefs, *directory))
			|| pending_snapshot_references_directory(*directory)
			|| loaded_floppy_paths_reference_directory(*directory)
			|| loaded_cd_paths_reference_directory(*directory)) {
			++directory;
			continue;
		}

		std::error_code error;
		std::filesystem::remove_all(*directory, error);
		if (error) {
			write_log(_T("RP9: could not remove unused temporary directory '%s': %s\n"),
				directory->string().c_str(), error.message().c_str());
			++directory;
		} else {
			write_log(_T("RP9: removed unused temporary directory: %s\n"), directory->string().c_str());
			directory = temporary_directories.erase(directory);
		}
	}
}

bool pending_snapshot_belongs_to_loaded_rp9()
{
	return !loaded_snapshot_path.empty()
		&& savestate_state == STATE_DORESTORE
		&& loaded_snapshot_path == savestate_fname;
}

std::string lowercase(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

void set_error(const std::string& value)
{
	last_error = value;
	write_log(_T("RP9: %s\n"), value.c_str());
}

void copy_path(TCHAR* destination, const std::size_t size, const std::string& source)
{
	if (size == 0)
		return;
	_tcsncpy(destination, source.c_str(), size - 1);
	destination[size - 1] = 0;
}

bool parse_integer(const std::string& value, int& result)
{
	if (value.empty())
		return false;
	int parsed = 0;
	const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
	if (error != std::errc {} || end != value.data() + value.size())
		return false;
	result = parsed;
	return true;
}

int system_rom_code(const std::string& value)
{
	const auto normalized = lowercase(value);
	if (normalized.empty())
		return -1;
	if (normalized == "1.0" || normalized == "1.0.0" || normalized == "kickstart 1.0")
		return 100;
	if (normalized == "1.1" || normalized == "1.1.0" || normalized == "kickstart 1.1")
		return 110;
	if (normalized == "1.2" || normalized == "1.2.0" || normalized == "kickstart 1.2")
		return 120;
	if (normalized == "1.3" || normalized == "1.3.0" || normalized == "kickstart 1.3")
		return 130;
	if (normalized == "2.04" || normalized == "kickstart 2.04")
		return 204;
	if (normalized == "2.05" || normalized == "kickstart 2.05")
		return 205;
	if (normalized == "3.0" || normalized == "3.0.0" || normalized == "kickstart 3.0")
		return 300;
	if (normalized == "3.1" || normalized == "3.1.0" || normalized == "kickstart 3.1")
		return 310;
	if (normalized == "310-cd32" || normalized == "3.1-cd32" || normalized == "kickstart 3.1 cd32")
		return RP9_SYSTEM_ROM_310_CD32;
	int result = -1;
	return parse_integer(normalized, result) ? result : invalid_system_rom;
}

struct ArchiveMediaFingerprint
{
	std::uint64_t size;
	std::uint32_t crc;
};

void update_deployment_hash(std::uint64_t& hash, const unsigned char value)
{
	hash ^= value;
	hash *= 1099511628211ULL;
}

void update_deployment_hash(std::uint64_t& hash, const std::uint64_t value)
{
	for (unsigned int shift = 0; shift < 64; shift += 8)
		update_deployment_hash(hash, static_cast<unsigned char>(value >> shift));
}

bool deployment_id(unzFile archive, const char* manifest, const std::size_t size,
	const rp9::Manifest& parsed_manifest, std::string& result)
{
	std::uint64_t hash = 14695981039346656037ULL;
	for (std::size_t index = 0; index < size; ++index)
		update_deployment_hash(hash, static_cast<unsigned char>(manifest[index]));

	std::unordered_set<std::string> deployed_paths;
	for (const auto& media : parsed_manifest.media) {
		if (!media.deploy && lowercase(media.root) != "deploy")
			continue;
		std::string normalized;
		if (!rp9::normalize_package_path(media.path, normalized)) {
			set_error("Unsafe deployed-media path in RP9 manifest: " + media.path);
			return false;
		}
		deployed_paths.emplace(lowercase(normalized));
	}

	std::unordered_map<std::string, ArchiveMediaFingerprint> fingerprints;
	if (!deployed_paths.empty()) {
		unz_global_info global {};
		if (unzGetGlobalInfo(archive, &global) != UNZ_OK || global.number_entry > maximum_archive_entries) {
			set_error("RP9 archive has an invalid or excessive number of entries");
			return false;
		}
		if (unzGoToFirstFile(archive) != UNZ_OK) {
			set_error("RP9 archive contains no readable entries");
			return false;
		}

		for (unsigned long index = 0; index < global.number_entry; ++index) {
			unz_file_info info {};
			if (unzGetCurrentFileInfo(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK
				|| info.size_filename == 0 || info.size_filename > maximum_entry_name) {
				set_error("RP9 archive contains an invalid entry");
				return false;
			}
			std::vector<char> name(static_cast<std::size_t>(info.size_filename) + 1);
			if (unzGetCurrentFileInfo(archive, &info, name.data(), static_cast<unsigned long>(name.size()),
				nullptr, 0, nullptr, 0) != UNZ_OK) {
				set_error("Could not read an RP9 archive entry name");
				return false;
			}
			const std::string entry_name(name.data(), static_cast<std::size_t>(info.size_filename));
			if (entry_name.find('\0') != std::string::npos) {
				set_error("RP9 archive contains an entry name with an embedded null byte");
				return false;
			}

			std::string normalized;
			if (!rp9::normalize_package_path(entry_name, normalized)) {
				set_error("Unsafe path in RP9 archive: " + entry_name);
				return false;
			}
			auto path_key = lowercase(normalized);
			if (!path_key.empty() && path_key.back() == '/')
				path_key.pop_back();
			const bool is_directory = entry_name.back() == '/' || entry_name.back() == '\\';
			if (!is_directory && deployed_paths.find(path_key) != deployed_paths.end()) {
				if (!fingerprints.emplace(path_key, ArchiveMediaFingerprint {
					static_cast<std::uint64_t>(info.uncompressed_size), static_cast<std::uint32_t>(info.crc)
				}).second) {
					set_error("RP9 archive contains a duplicate deployed-media path: " + entry_name);
					return false;
				}
			}
			if (index + 1 < global.number_entry && unzGoToNextFile(archive) != UNZ_OK) {
				set_error("RP9 archive ended before all entries were read");
				return false;
			}
		}
	}

	// ZIP CRC and size describe the uncompressed source media, so the identity
	// remains stable after moves, repacking, and guest writes to the deployed
	// copy while changing when a package supplies different persistent media.
	for (const auto& media : parsed_manifest.media) {
		if (!media.deploy && lowercase(media.root) != "deploy")
			continue;
		std::string normalized;
		if (!rp9::normalize_package_path(media.path, normalized))
			return false;
		const auto path_key = lowercase(normalized);
		update_deployment_hash(hash, static_cast<std::uint64_t>(path_key.size()));
		for (const auto value : path_key)
			update_deployment_hash(hash, static_cast<unsigned char>(value));
		const auto fingerprint = fingerprints.find(path_key);
		update_deployment_hash(hash, static_cast<unsigned char>(fingerprint != fingerprints.end()));
		if (fingerprint != fingerprints.end()) {
			update_deployment_hash(hash, fingerprint->second.size);
			update_deployment_hash(hash, static_cast<std::uint64_t>(fingerprint->second.crc));
		}
	}
	std::array<char, 16> buffer {};
	const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), hash, 16);
	if (error != std::errc {}) {
		set_error("Could not create the RP9 deployment identity");
		return false;
	}
	result = std::string(16 - static_cast<std::size_t>(end - buffer.data()), '0')
		+ std::string(buffer.data(), end);
	return true;
}

bool deployed_media_path(const rp9::Media& media, const std::string& package_id,
	std::filesystem::path& destination, std::string& normalized)
{
	if (!rp9::normalize_package_path(media.path, normalized))
		return false;
	const char* media_directory = nullptr;
	switch (media.type) {
	case rp9::MediaType::Floppy:
		media_directory = "adf";
		break;
	case rp9::MediaType::HardDrive:
		media_directory = "hdf";
		break;
	case rp9::MediaType::Cd:
		media_directory = "cd";
		break;
	case rp9::MediaType::Tape:
		media_directory = "tape";
		break;
	case rp9::MediaType::Snapshot:
		media_directory = "snapshot";
		break;
	default:
		return false;
	}
	destination = std::filesystem::path(get_rp9_path()) / "Shared" / media_directory
		/ package_id / std::filesystem::path(normalized);
	return true;
}

std::unordered_map<std::string, std::filesystem::path> find_existing_deployments(const rp9::Manifest& manifest,
	const std::string& package_id)
{
	std::unordered_map<std::string, std::filesystem::path> result;
	for (const auto& media : manifest.media) {
		if (!media.deploy && lowercase(media.root) != "deploy")
			continue;
		std::filesystem::path destination;
		std::string normalized;
		if (!deployed_media_path(media, package_id, destination, normalized))
			continue;
		std::error_code error;
		if (std::filesystem::is_regular_file(destination, error) && !error)
			result.emplace(lowercase(normalized), destination);
	}
	return result;
}

bool set_default_system(uae_prefs* prefs, const rp9::Manifest& manifest)
{
	// Build RP9 preferences without clearing the live input-device store. The
	// caller commits that global reset only after the complete package succeeds.
	const auto whdload_write_cache = whdload_prefs.write_cache;
	default_prefs(prefs, false, 0);
	whdload_prefs.write_cache = whdload_write_cache;
	inputdevice_joyport_config_store(prefs, _T("mouse"), 0, -1, -1, 0);
	inputdevice_joyport_config_store(prefs, _T("joy0"), 1, -1, -1, 0);
	const auto& system = manifest.system;
	const auto normalized_rom = lowercase(manifest.system_rom);
	const bool has_explicit_rom = !normalized_rom.empty();
	if (manifest.video == "ntsc" || manifest.video == "a-ntsc")
		prefs->ntscmode = true;
	else if (manifest.video == "pal" || manifest.video == "a-pal")
		prefs->ntscmode = false;
	if (system == "a-aros") {
		if (has_explicit_rom && normalized_rom != "aros-latest") {
			set_error("Unsupported RP9 system ROM '" + manifest.system_rom + "' for " + system);
			return false;
		}
		bip_a4000(prefs, -1);
		copy_path(prefs->romfile, MAX_DPATH, ":AROS");
		return true;
	}
	const auto rom = system_rom_code(manifest.system_rom);
	if (rom == invalid_system_rom) {
		set_error("Unsupported RP9 system ROM: " + manifest.system_rom);
		return false;
	}
	int configured = 0;
	auto rp9_model = rp9_system_model::a500;

	if (system == "a-1000" || system == "a1000") {
		configured = bip_a1000(prefs, rom);
		rp9_model = rp9_system_model::a1000;
	} else if (system == "a-500" || system == "a500") {
		configured = bip_a500(prefs, rom < 0 ? 130 : rom);
		rp9_model = rp9_system_model::a500;
	} else if (system == "a-500plus" || system == "a-500+" || system == "a500plus") {
		configured = bip_a500plus(prefs, rom);
		rp9_model = rp9_system_model::a500plus;
	} else if (system == "a-600" || system == "a600") {
		configured = bip_a600(prefs, rom);
		rp9_model = rp9_system_model::a600;
	} else if (system == "a-1200" || system == "a1200") {
		configured = bip_a1200(prefs, rom);
		rp9_model = rp9_system_model::a1200;
	} else if (system == "a-2000" || system == "a2000") {
		configured = bip_a2000(prefs, rom < 0 ? 130 : rom);
		rp9_model = rp9_system_model::a2000;
	} else if (system == "a-3000" || system == "a3000") {
		configured = bip_a3000(prefs, rom);
		rp9_model = rp9_system_model::a3000;
	} else if (system == "a-4000" || system == "a4000" || system == "a-4xxx") {
		configured = bip_a4000(prefs, rom);
		rp9_model = rp9_system_model::a4000;
	} else if (system == "a-walker") {
		// Walker used AGA, IDE and a 68030; A1200 is Amiberry's closest
		// canonical base before the manifest's CPU and RAM overrides are applied.
		configured = bip_a1200(prefs, rom);
		rp9_model = rp9_system_model::a1200;
	} else if (system == "cdtv" || system == "a-cdtv") {
		configured = bip_cdtv(prefs, rom);
		rp9_model = rp9_system_model::cdtv;
	} else if (system == "cd32" || system == "a-cd32") {
		configured = bip_cd32(prefs, rom);
		rp9_model = rp9_system_model::cd32;
	} else {
		set_error("Unsupported RP9 system: " + system);
		return false;
	}
	if (has_explicit_rom)
		configured = configure_rp9_system_rom(prefs, rp9_model, rom);
	if (has_explicit_rom && !configured) {
		set_error("Required RP9 system ROM '" + manifest.system_rom
			+ "' is unavailable or incompatible with " + system);
		return false;
	}
	return true;
}

bool read_manifest(unzFile archive, std::vector<char>& data)
{
	if (unzLocateFile(archive, manifest_name, 2) != UNZ_OK) {
		set_error("Package does not contain rp9-manifest.xml");
		return false;
	}

	unz_file_info info {};
	if (unzGetCurrentFileInfo(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK
		|| info.uncompressed_size == 0 || info.uncompressed_size > maximum_manifest_size) {
		set_error("RP9 manifest is empty or exceeds the 4 MiB safety limit");
		return false;
	}
	if (unzOpenCurrentFile(archive) != UNZ_OK) {
		set_error("Could not open RP9 manifest");
		return false;
	}

	data.resize(static_cast<std::size_t>(info.uncompressed_size) + 1);
	std::size_t offset = 0;
	while (offset < info.uncompressed_size) {
		const auto remaining = static_cast<unsigned int>(std::min<std::size_t>(
			info.uncompressed_size - offset, extraction_buffer_size));
		const auto bytes = unzReadCurrentFile(archive, data.data() + offset, remaining);
		if (bytes <= 0) {
			unzCloseCurrentFile(archive);
			set_error("Could not read complete RP9 manifest");
			return false;
		}
		offset += static_cast<std::size_t>(bytes);
	}
	const auto close_result = unzCloseCurrentFile(archive);
	if (close_result != UNZ_OK) {
		set_error("RP9 manifest failed its integrity check");
		return false;
	}
	data[offset] = 0;
	return true;
}

std::filesystem::path create_extraction_directory()
{
	std::error_code error;
	auto root = std::filesystem::path(get_rp9_path()) / "tmp";
	std::filesystem::create_directories(root, error);
	if (error)
		return {};

	const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	for (int attempt = 0; attempt < 16; ++attempt) {
		auto directory = root / ("rp9-" + std::to_string(now) + "-"
			+ std::to_string(directory_sequence.fetch_add(1)));
		if (std::filesystem::create_directory(directory, error))
			return directory;
		error.clear();
	}
	return {};
}

bool register_extracted_path(std::unordered_map<std::string, std::filesystem::path>& files,
	const std::string& package_path, const std::filesystem::path& host_path)
{
	const auto [existing, inserted] = files.emplace(lowercase(package_path), host_path);
	if (inserted || existing->second.lexically_normal() == host_path.lexically_normal())
		return true;
	set_error("RP9 archive contains colliding paths: " + package_path);
	return false;
}

bool register_extracted_directories(std::unordered_map<std::string, std::filesystem::path>& files,
	const std::filesystem::path& root, std::filesystem::path relative_directory)
{
	while (!relative_directory.empty()) {
		if (!register_extracted_path(files, relative_directory.generic_string(), root / relative_directory))
			return false;
		relative_directory = relative_directory.parent_path();
	}
	return true;
}

bool extract_archive(unzFile archive, const std::filesystem::path& directory,
	std::unordered_map<std::string, std::filesystem::path>& files,
	const std::unordered_map<std::string, std::filesystem::path>& skipped_files)
{
	unz_global_info global {};
	if (unzGetGlobalInfo(archive, &global) != UNZ_OK || global.number_entry > maximum_archive_entries) {
		set_error("RP9 archive has an invalid or excessive number of entries");
		return false;
	}
	if (unzGoToFirstFile(archive) != UNZ_OK) {
		set_error("RP9 archive contains no readable entries");
		return false;
	}

	std::vector<char> buffer(extraction_buffer_size);
	std::uint64_t total_size = 0;
	std::unordered_set<std::string> seen_paths;
	for (unsigned long index = 0; index < global.number_entry; ++index) {
		unz_file_info info {};
		if (unzGetCurrentFileInfo(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK
			|| info.size_filename == 0 || info.size_filename > maximum_entry_name) {
			set_error("RP9 archive contains an invalid entry");
			return false;
		}
		std::vector<char> name(static_cast<std::size_t>(info.size_filename) + 1);
		if (unzGetCurrentFileInfo(archive, &info, name.data(), static_cast<unsigned long>(name.size()),
			nullptr, 0, nullptr, 0) != UNZ_OK) {
			set_error("Could not read an RP9 archive entry name");
			return false;
		}
		const std::string entry_name(name.data(), static_cast<std::size_t>(info.size_filename));
		if (entry_name.find('\0') != std::string::npos) {
			set_error("RP9 archive contains an entry name with an embedded null byte");
			return false;
		}

		const bool is_directory = entry_name.back() == '/' || entry_name.back() == '\\';
		std::string normalized;
		if (!rp9::normalize_package_path(entry_name, normalized)) {
			set_error("Unsafe path in RP9 archive: " + entry_name);
			return false;
		}
		if (is_directory && normalized.back() == '/')
			normalized.pop_back();
		auto path_key = lowercase(normalized);
		if (!seen_paths.emplace(path_key).second) {
			set_error("RP9 archive contains a duplicate path: " + entry_name);
			return false;
		}
		const auto destination = directory / std::filesystem::path(normalized);
		std::error_code error;
		if (is_directory) {
			std::filesystem::create_directories(destination, error);
			if (!error && !register_extracted_directories(files, directory, std::filesystem::path(normalized)))
				return false;
		} else if (lowercase(normalized) != manifest_name) {
			std::filesystem::create_directories(destination.parent_path(), error);
			if (error) {
				set_error("Could not create an RP9 extraction directory: " + error.message());
				return false;
			}
			if (!register_extracted_directories(files, directory,
				std::filesystem::path(normalized).parent_path()))
				return false;
			if (skipped_files.find(path_key) != skipped_files.end()) {
				write_log(_T("RP9: skipping embedded copy of deployed media: %s\n"), normalized.c_str());
			} else {
				if (total_size > maximum_extracted_size - info.uncompressed_size) {
					set_error("RP9 package exceeds the 16 GiB extraction safety limit");
					return false;
				}
				total_size += info.uncompressed_size;
				if (std::filesystem::exists(destination, error)) {
					set_error("RP9 archive contains paths that collide on this filesystem: " + entry_name);
					return false;
				}
				if (error) {
					set_error("Could not validate an RP9 extraction path: " + error.message());
					return false;
				}
				auto output = uae_fopen(destination.string().c_str(), _T("wbe"));
				if (!output) {
					set_error("Could not extract RP9 entry: " + normalized);
					return false;
				}
				if (unzOpenCurrentFile(archive) != UNZ_OK) {
					fclose(output);
					set_error("Could not open compressed RP9 entry: " + normalized);
					return false;
				}
				std::uint64_t written = 0;
				for (;;) {
					const auto bytes = unzReadCurrentFile(archive, buffer.data(), static_cast<unsigned int>(buffer.size()));
					if (bytes < 0) {
						unzCloseCurrentFile(archive);
						fclose(output);
						set_error("Error while extracting RP9 entry: " + normalized);
						return false;
					}
					if (bytes == 0)
						break;
					if (written > info.uncompressed_size
						|| static_cast<std::uint64_t>(bytes) > info.uncompressed_size - written) {
						unzCloseCurrentFile(archive);
						fclose(output);
						set_error("RP9 entry expands beyond its declared size: " + normalized);
						return false;
					}
					if (fwrite(buffer.data(), 1, static_cast<std::size_t>(bytes), output)
						!= static_cast<std::size_t>(bytes)) {
						unzCloseCurrentFile(archive);
						fclose(output);
						set_error("Could not write RP9 entry: " + normalized);
						return false;
					}
					written += static_cast<std::uint64_t>(bytes);
				}
				const auto output_close_result = fclose(output);
				if (unzCloseCurrentFile(archive) != UNZ_OK || output_close_result != 0
					|| written != info.uncompressed_size) {
					set_error("RP9 entry failed its integrity check: " + normalized);
					return false;
				}
				if (!register_extracted_path(files, normalized, destination))
					return false;
			}
		}
		if (error) {
			set_error("Could not create an RP9 extraction path: " + error.message());
			return false;
		}
		if (index + 1 < global.number_entry && unzGoToNextFile(archive) != UNZ_OK) {
			set_error("RP9 archive ended before all entries were read");
			return false;
		}
	}
	return true;
}

std::filesystem::path resolve_media_path(const rp9::Media& media,
	const std::unordered_map<std::string, std::filesystem::path>& files,
	const std::string& deployment_id)
{
	std::string normalized;
	const auto root = lowercase(media.root);
	if (media.deploy || root == "deploy") {
		std::filesystem::path destination;
		if (!deployed_media_path(media, deployment_id, destination, normalized))
			return {};
		std::error_code error;
		if (std::filesystem::exists(destination, error)) {
			if (!error && std::filesystem::is_regular_file(destination, error) && !error) {
				write_log(_T("RP9: reusing deployed media: %s\n"), destination.string().c_str());
				return destination;
			}
			set_error("RP9 deployed-media destination is not a regular file: " + destination.string());
			return {};
		}
		if (error) {
			set_error("Could not inspect RP9 deployed-media destination: " + error.message());
			return {};
		}
		const auto found = files.find(lowercase(normalized));
		if (found == files.end())
			return {};

		std::filesystem::create_directories(destination.parent_path(), error);
		if (error) {
			set_error("Could not create the RP9 deployed-media directory: " + error.message());
			return {};
		}

		auto temporary = destination;
		temporary += ".tmp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
			+ "-" + std::to_string(directory_sequence.fetch_add(1));
		std::filesystem::copy_file(found->second, temporary, std::filesystem::copy_options::none, error);
		if (error) {
			std::error_code cleanup_error;
			std::filesystem::remove(temporary, cleanup_error);
			set_error("Could not deploy RP9 media: " + error.message());
			return {};
		}
		std::filesystem::rename(temporary, destination, error);
		if (error) {
			// A second process may have completed the same deployment first.
			std::error_code destination_error;
			if (std::filesystem::is_regular_file(destination, destination_error) && !destination_error) {
				std::filesystem::remove(temporary, destination_error);
				write_log(_T("RP9: reusing concurrently deployed media: %s\n"), destination.string().c_str());
				return destination;
			}
			std::error_code cleanup_error;
			std::filesystem::remove(temporary, cleanup_error);
			set_error("Could not finish deploying RP9 media: " + error.message());
			return {};
		}
		write_log(_T("RP9: deployed media to: %s\n"), destination.string().c_str());
		return destination;
	}
	if (root.empty() || root == "embedded") {
		if (!rp9::normalize_package_path(media.path, normalized))
			return {};
		const auto found = files.find(lowercase(normalized));
		return found == files.end() ? std::filesystem::path {} : found->second;
	}

	std::error_code error;
	if (root == "absolute" || root == "external") {
		const std::filesystem::path path(media.path);
		if (!path.is_absolute()) {
			set_error("RP9 external media path is not absolute on this host: " + media.path);
			return {};
		}

		const auto status = std::filesystem::status(path, error);
		if (error) {
			set_error("Could not inspect RP9 external media path '" + media.path + "': " + error.message());
			return {};
		}
		if (!std::filesystem::exists(status)) {
			set_error("RP9 external media was not found: " + media.path);
			return {};
		}
		if (!std::filesystem::is_regular_file(status) && !std::filesystem::is_directory(status)) {
			set_error("RP9 external media is not a regular file or directory: " + media.path);
			return {};
		}

		const auto resolved = path.lexically_normal();
		write_log(_T("RP9: resolved external media: %s\n"), resolved.string().c_str());
		return resolved;
	}
	if (root == "data" || root == "shared") {
		const auto base = std::filesystem::path(get_rp9_path());
		std::filesystem::path relative_path;
		if (!media.path.empty()) {
			if (!rp9::normalize_package_path(media.path, normalized))
				return {};
			relative_path = std::filesystem::path(normalized);
		}
		std::vector<std::filesystem::path> candidates;
		if (root == "shared") {
			// The empty shared root is the user-facing Shared volume, stored
			// under Shared/dir/Local in the current Amiga Files layout.
			if (relative_path.empty()) {
				candidates = { base / "data" / "Shared" / "dir" / "Local",
					base / "Shared" / "dir" / "Local" };
			}
			candidates.emplace_back(base / "data" / "Shared" / relative_path);
			candidates.emplace_back(base / "Shared" / relative_path);
		} else {
			candidates = { base / "data" / relative_path, base / relative_path };
		}
		for (const auto& candidate : candidates) {
			if (std::filesystem::exists(candidate, error))
				return candidate;
			error.clear();
		}
	}
	return {};
}

std::filesystem::path prepare_undoable_hardfile(const rp9::Media& media,
	const std::filesystem::path& source, const std::filesystem::path& extraction_directory,
	const int device_number)
{
	// Embedded hardfiles already live in the disposable extraction directory.
	// Read-only media cannot accumulate changes, so neither case needs a copy.
	if (!media.undo || media.readonly
		|| directory_contains_path(extraction_directory, source.string().c_str()))
		return source;

	std::error_code error;
	if (!std::filesystem::is_regular_file(source, error) || error) {
		set_error("RP9 undo is only supported for hardfile images: " + media.path);
		return {};
	}

	const auto undo_directory = extraction_directory / "undo";
	std::filesystem::create_directories(undo_directory, error);
	if (error) {
		set_error("Could not create the RP9 hardfile undo directory: " + error.message());
		return {};
	}

	auto filename = source.filename().string();
	if (filename.empty())
		filename = "disk.hdf";
	const auto destination = undo_directory
		/ ("harddrive-" + std::to_string(device_number) + "-" + filename);
	std::filesystem::copy_file(source, destination, std::filesystem::copy_options::none, error);
	if (error) {
		set_error("Could not create the RP9 hardfile undo copy: " + error.message());
		return {};
	}

	write_log(_T("RP9: using temporary undo copy for hardfile: %s\n"), destination.string().c_str());
	return destination;
}

void apply_compatibility(uae_prefs* prefs, const rp9::Manifest& manifest)
{
	for (const auto& value : manifest.compatibility) {
		if (value == "flexible-blitter-immediate")
			prefs->immediate_blits = true;
		else if (value == "turbo-floppy")
			prefs->floppy_speed = 400;
		else if (value == "flexible-sprite-collisions-spritesplayfield")
			prefs->collision_level = 2;
		else if (value == "flexible-sprite-collisions-spritesonly")
			prefs->collision_level = 1;
		else if (value == "flexible-sound")
			prefs->produce_sound = 2;
		else if (value == "mouse-integration" || value == "mouseintegration")
			prefs->input_tablet = TABLET_MOUSEHACK;
		else if (value == "turbo-cpu")
			prefs->m68k_speed = -1;
		else if (value == "jit") {
#ifdef JIT
			prefs->cachesize = MAX_JIT_CACHE;
			prefs->address_space_24 = false;
			prefs->compfpu = true;
#endif
		} else if (value == "flexible-cpu-cycles") {
			prefs->cpu_compatible = false;
		} else if (value != "flexible-blitter-cycles"
			&& value != "flexible-cpu-compatibility"
			&& value != "hostio"
			&& value != "nosavestate"
			&& value != "flexible-maxhorizontal-nohires"
			&& value != "flexible-maxhorizontal-nosuperhires"
			&& value != "flexible-maxvertical-nointerlace") {
			write_log(_T("RP9: unsupported compatibility hint '%s'\n"), value.c_str());
		}
	}
}

#ifdef JIT
bool manifest_requests_jit(const rp9::Manifest& manifest)
{
	if (std::find(manifest.compatibility.begin(), manifest.compatibility.end(), "jit")
		!= manifest.compatibility.end())
		return true;
	return std::any_of(manifest.peripherals.begin(), manifest.peripherals.end(), [](const auto& peripheral) {
		return peripheral.name == "jit";
	});
}
#endif

void apply_ram(uae_prefs* prefs, const rp9::Manifest& manifest)
{
	for (const auto& ram : manifest.ram) {
		if (ram.type == "chip")
			prefs->chipmem.size = static_cast<uae_u32>(std::min<std::uint64_t>(ram.size, UINT32_MAX));
		else if (ram.type == "fast")
			prefs->fastmem[0].size = static_cast<uae_u32>(std::min<std::uint64_t>(ram.size, UINT32_MAX));
		else if (ram.type == "z3") {
			prefs->address_space_24 = false;
			prefs->z3fastmem[0].size = static_cast<uae_u32>(std::min<std::uint64_t>(ram.size, UINT32_MAX));
		}
		else if (ram.type == "slow" || ram.type == "bogo")
			prefs->bogomem.size = static_cast<uae_u32>(std::min<std::uint64_t>(ram.size, UINT32_MAX));
		else
			write_log(_T("RP9: unsupported RAM type '%s'\n"), ram.type.c_str());
	}
}

bool apply_peripherals(uae_prefs* prefs, const rp9::Manifest& manifest)
{
	for (const auto& peripheral : manifest.peripherals) {
		if (peripheral.name == "floppy") {
			if (peripheral.unit < 0 || peripheral.unit >= 4) {
				write_log(_T("RP9: ignoring invalid floppy unit %d\n"), peripheral.unit);
				continue;
			}
			prefs->floppyslots[peripheral.unit].dfxtype = peripheral.type == "hd" ? DRV_35_HD : DRV_35_DD;
			prefs->nr_floppies = std::max(prefs->nr_floppies, peripheral.unit + 1);
		} else if (peripheral.name == "a-501") {
			prefs->bogomem.size = 0x00080000;
		} else if (peripheral.name == "cpu") {
			if (!peripheral.type.empty()) {
				int cpu = 0;
				if (parse_integer(peripheral.type, cpu)
					&& (cpu == 68000 || cpu == 68010 || cpu == 68020 || cpu == 68030 || cpu == 68040 || cpu == 68060)) {
					prefs->cpu_model = cpu;
					if (cpu > 68020)
						prefs->address_space_24 = false;
					if (cpu == 68040 || cpu == 68060)
						prefs->fpu_model = cpu;
				} else {
					write_log(_T("RP9: unsupported CPU type '%s'\n"), peripheral.type.c_str());
				}
			}
			if (peripheral.speed == "max")
				prefs->m68k_speed = -1;
		} else if (peripheral.name == "fpu") {
			if (peripheral.type == "68881")
				prefs->fpu_model = 68881;
			else if (peripheral.type == "68882")
				prefs->fpu_model = 68882;
		} else if (peripheral.name == "jit") {
#ifdef JIT
			if (peripheral.has_memory)
				prefs->cachesize = static_cast<int>(std::clamp<std::uint64_t>(peripheral.memory / 1024, 1, MAX_JIT_CACHE));
			else
				prefs->cachesize = MAX_JIT_CACHE;
			if (peripheral.has_fpu)
				prefs->compfpu = peripheral.fpu;
			prefs->address_space_24 = false;
#endif
		} else if (peripheral.name == "ppc" && peripheral.type == "cyberstorm") {
#ifdef WITH_PPC
			// RetroPlatform's "cyberstorm" PPC profile denotes the complete
			// CyberStorm PPC accelerator, including its 68060 and SCSI controller.
			prefs->cpu_model = 68060;
			prefs->fpu_model = 68060;
			prefs->address_space_24 = false;
			prefs->ppc_mode = 1;
			cpuboard_setboard(prefs, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_PPC);
			if (peripheral.has_memory)
				prefs->cpuboardmem1.size = static_cast<uae_u32>(std::min<std::uint64_t>(
					peripheral.memory, UINT32_MAX));
#else
			set_error("This RP9 package requires CyberStorm PPC support, which is not enabled in this build");
			return false;
#endif
		} else if (peripheral.name == "rtg" && peripheral.type == "picasso-iv") {
			prefs->address_space_24 = false;
			prefs->rtgboards[0].rtgmem_type = GFXBOARD_ID_PICASSO4_Z3;
			prefs->rtgboards[0].rtgmem_size = static_cast<uae_u32>(std::min<std::uint64_t>(
				peripheral.has_memory ? peripheral.memory : 4 * 1024 * 1024, UINT32_MAX));
		} else if ((peripheral.name == "scsi" && peripheral.type == "cyberstorm")
			|| (peripheral.name == "optical" && peripheral.type == "cd")) {
			// These controllers are part of the CyberStorm/CD machine profiles.
		} else if (peripheral.name == "input-joystick" && peripheral.unit >= 0 && peripheral.unit < MAX_JPORTS) {
			prefs->jports[peripheral.unit].mode = JSEM_MODE_JOYSTICK;
		} else if (peripheral.name == "input-mouse" && peripheral.unit >= 0 && peripheral.unit < MAX_JPORTS) {
			prefs->jports[peripheral.unit].mode = JSEM_MODE_MOUSE;
		} else if (peripheral.name == "input-cd32" && peripheral.unit >= 0 && peripheral.unit < MAX_JPORTS) {
			prefs->jports[peripheral.unit].mode = JSEM_MODE_JOYSTICK_CD32;
		} else if (peripheral.name == "automount-removable") {
			prefs->automount_removable = true;
		} else if (peripheral.name == "keyboard") {
			const auto& layout = peripheral.layout;
			if (layout.empty() || layout == "en" || layout == "en-us" || layout == "en-gb")
				prefs->keyboard_lang = KBD_LANG_US;
			else if (layout == "da" || layout == "da-dk" || layout == "dk")
				prefs->keyboard_lang = KBD_LANG_DK;
			else if (layout == "de" || layout == "de-de")
				prefs->keyboard_lang = KBD_LANG_DE;
			else if (layout == "sv" || layout == "sv-se" || layout == "se")
				prefs->keyboard_lang = KBD_LANG_SE;
			else if (layout == "fr" || layout == "fr-fr")
				prefs->keyboard_lang = KBD_LANG_FR;
			else if (layout == "it" || layout == "it-it")
				prefs->keyboard_lang = KBD_LANG_IT;
			else if (layout == "es" || layout == "es-es")
				prefs->keyboard_lang = KBD_LANG_ES;
			else
				write_log(_T("RP9: unsupported keyboard layout '%s'\n"), layout.c_str());
		} else if (peripheral.name != "silent-drives"
			&& peripheral.name != "filter-interpolation"
			&& peripheral.name != "printer") {
			write_log(_T("RP9: unsupported peripheral '%s'\n"), peripheral.name.c_str());
		}
	}
	return true;
}

void apply_video_and_clip(uae_prefs* prefs, const rp9::Manifest& manifest)
{
	if (manifest.video == "ntsc" || manifest.video == "a-ntsc")
		prefs->ntscmode = true;
	else if (manifest.video == "pal" || manifest.video == "a-pal")
		prefs->ntscmode = false;

	if (!manifest.has_clip)
		return;
	const auto hres = std::clamp(prefs->gfx_resolution, RES_LORES, RES_SUPERHIRES);
	const auto vres = std::clamp(prefs->gfx_vresolution, VRES_NONDOUBLE, VRES_DOUBLE);
	const auto horizontal_scale = 1U << (RES_SUPERHIRES - hres);
	const auto vertical_scale = 1U << (VRES_DOUBLE - vres);
	const int full_width = AMIGA_WIDTH_MAX << hres;
	const int full_height = (prefs->ntscmode ? AMIGA_HEIGHT_MAX_NTSC : AMIGA_HEIGHT_MAX_PAL) << vres;
	const int width = static_cast<int>(std::min<std::uint64_t>(
		std::max(1U, manifest.clip.width / horizontal_scale), full_width));
	const int height = static_cast<int>(std::min<std::uint64_t>(
		std::max(1U, manifest.clip.height / vertical_scale), full_height));
	const auto relative_left = (static_cast<std::int64_t>(manifest.clip.left) - 112)
		/ static_cast<std::int64_t>(horizontal_scale);
	// RP9 top coordinates expose the standard interlaced vertical-blanking
	// interval. Amiberry's native render surface begins after that interval.
	const auto vertical_blanking = prefs->ntscmode ? 42 : 52;
	const auto relative_top = (static_cast<std::int64_t>(manifest.clip.top) - vertical_blanking)
		/ static_cast<std::int64_t>(vertical_scale);
	// RP9 clip positions are absolute within the maximum visible Amiga area.
	// Only the documented 112-SuperHires-unit RP9 left-origin correction is
	// removed; these are not offsets relative to a centered crop rectangle.
	const auto horizontal_offset = std::clamp<std::int64_t>(
		relative_left, 0, std::max(0, full_width - width));
	const auto vertical_offset = std::clamp<std::int64_t>(
		relative_top, 0, std::max(0, full_height - height));

	prefs->gfx_auto_crop = false;
	prefs->gfx_manual_crop = true;
	prefs->gfx_manual_crop_width = width;
	prefs->gfx_manual_crop_height = height;
	prefs->gfx_horizontal_offset = static_cast<int>(horizontal_offset);
	prefs->gfx_vertical_offset = static_cast<int>(vertical_offset);
}

bool is_rdb_hardfile(const std::filesystem::path& path);

bool add_harddrive(uae_prefs* prefs, const std::filesystem::path& path, const std::string& volume_name,
	const bool readonly, const int boot_priority, int& device_number)
{
	uaedev_config_info config {};
	std::error_code error;
	const bool directory = std::filesystem::is_directory(path, error);
	uci_set_defaults(&config, !directory && is_rdb_hardfile(path));
	config.type = directory ? UAEDEV_DIR : UAEDEV_HDF;
	config.readonly = readonly;
	config.bootpri = boot_priority;
	snprintf(config.devname, sizeof config.devname, _T("DH%d"), device_number++);
	copy_path(config.rootdir, sizeof config.rootdir / sizeof(TCHAR), path.string());
	if (directory) {
		const auto name = volume_name.empty() ? path.filename().string() : volume_name;
		copy_path(config.volname, sizeof config.volname / sizeof(TCHAR), name.empty() ? "RP9" : name);
	}
	return add_filesys_config(prefs, -1, &config) != nullptr;
}

std::filesystem::path find_boot_media(const rp9::Boot& boot)
{
	const auto base = std::filesystem::path(get_rp9_path());
	const auto filename = "workbench-" + boot.value;
	std::vector<std::filesystem::path> candidates;
	if (boot.type == "hdf") {
		candidates = { base / (filename + ".hdf"),
			base / "Shared" / "hdf" / (filename + ".hdf"),
			base / "data" / "Shared" / "hdf" / (filename + ".hdf"),
			base / "Shared" / "HDF" / ("AWB" + boot.value + ".HDF"),
			base / "Shared" / "hdf" / ("awb" + boot.value + ".hdf"),
			base / "data" / "Shared" / "HDF" / ("AWB" + boot.value + ".HDF"),
			base / "data" / "Shared" / "hdf" / ("awb" + boot.value + ".hdf") };
	} else if (boot.type == "adf") {
		candidates = { base / (filename + ".adf"), base / "Shared" / "ADF" / ("AWB" + boot.value + ".ADF"),
			base / "Shared" / "adf" / ("awb" + boot.value + ".adf"),
			base / "Shared" / "adf" / ("amiga-os-" + boot.value + "-workbench.adf"),
			base / "Shared" / "ADF" / ("amiga-os-" + boot.value + "-workbench.adf"),
			base / "data" / "Shared" / "ADF" / ("AWB" + boot.value + ".ADF"),
			base / "data" / "Shared" / "adf" / ("amiga-os-" + boot.value + "-workbench.adf"),
			base / "data" / "Shared" / "ADF" / ("amiga-os-" + boot.value + "-workbench.adf") };
	} else if (boot.type == "dir") {
		candidates = { base / filename, base / "Shared" / "dir" / "System",
			base / "data" / "Shared" / "dir" / "System" };
	}
	std::error_code error;
	for (const auto& candidate : candidates) {
		if (std::filesystem::exists(candidate, error))
			return candidate;
		error.clear();
	}
	return {};
}

bool apply_boot(uae_prefs* prefs, const rp9::Manifest& manifest, int& device_number)
{
	if (!manifest.has_boot || manifest.boot.type.empty())
		return true;
	if (manifest.boot.type != "hdf" && manifest.boot.type != "adf" && manifest.boot.type != "dir") {
		set_error("Unsupported RP9 boot media type: " + manifest.boot.type);
		return false;
	}
	if (manifest.boot.value.empty()
		|| !std::all_of(manifest.boot.value.begin(), manifest.boot.value.end(), [](const unsigned char ch) {
			return std::isalnum(ch) || ch == '.' || ch == '-' || ch == '_';
		})) {
		set_error("RP9 boot image version contains invalid characters");
		return false;
	}
	const auto path = find_boot_media(manifest.boot);
	if (path.empty()) {
		set_error("RP9 requires missing " + manifest.boot.type + " boot media version " + manifest.boot.value
			+ " in the RP9/Shared data paths");
		return false;
	}
	if (manifest.boot.type == "adf") {
		prefs->nr_floppies = std::max(prefs->nr_floppies, 1);
		copy_path(prefs->floppyslots[0].df, MAX_DPATH, path.string());
		copy_path(prefs->dfxlist[0], MAX_DPATH, path.string());
		prefs->floppyslots[0].forcedwriteprotect = manifest.boot.readonly;
		return true;
	}
	const auto volume_name = manifest.boot.type == "dir" ? "System" : std::string {};
	if (!add_harddrive(prefs, path, volume_name, manifest.boot.readonly, 127, device_number)) {
		set_error("Could not attach RP9 boot image: " + path.string());
		return false;
	}
	return true;
}

bool is_rdb_hardfile(const std::filesystem::path& path)
{
	auto file = zfile_fopen(path.string().c_str(), _T("rb"), ZFD_NORMAL);
	if (!file)
		return false;
	std::array<unsigned char, 512> block {};
	bool found = false;
	for (int index = 0; index < 16; ++index) {
		if (zfile_fread(block.data(), 1, block.size(), file) != block.size())
			break;
		if (memcmp(block.data(), "RDSK", 4) == 0 || memcmp(block.data(), "CDSK", 4) == 0) {
			found = true;
			break;
		}
	}
	zfile_fclose(file);
	return found;
}

bool apply_media(uae_prefs* prefs, const rp9::Manifest& manifest,
	const std::unordered_map<std::string, std::filesystem::path>& files,
	const std::string& deployment_id, const std::filesystem::path& extraction_directory,
	std::string& snapshot_path,
	std::vector<std::string>& floppy_paths,
	std::vector<std::string>& cd_paths)
{
	int device_number = 0;
	bool cd_attached = false;
	bool snapshot_attached = false;
	if (!apply_boot(prefs, manifest, device_number))
		return false;
	const bool boot_floppy_attached = manifest.has_boot && manifest.boot.type == "adf";
	if (boot_floppy_attached)
		floppy_paths.emplace_back(prefs->floppyslots[0].df);
	int floppy_count = boot_floppy_attached ? 1 : 0;
	std::array<bool, 4> floppy_drive_assigned {};
	floppy_drive_assigned[0] = boot_floppy_attached;

	std::vector<const rp9::Media*> ordered_media;
	ordered_media.reserve(manifest.media.size());
	for (const auto& media : manifest.media)
		ordered_media.emplace_back(&media);
	// Priority 1 is the first attachment/insertion preference. Preserve XML
	// order when priorities are equal or omitted.
	std::stable_sort(ordered_media.begin(), ordered_media.end(), [](const auto* left, const auto* right) {
		if (left->has_priority != right->has_priority)
			return left->has_priority;
		return left->has_priority && left->priority < right->priority;
	});

	for (const auto* media_entry : ordered_media) {
		const auto& media = *media_entry;
		const auto path = resolve_media_path(media, files, deployment_id);
		if (path.empty()) {
			if (last_error.empty())
				set_error("RP9 media was not found or uses an unsupported root: " + media.path);
			return false;
		}
		const auto path_string = path.string();
		switch (media.type) {
		case rp9::MediaType::Floppy:
			floppy_paths.emplace_back(path_string);
			if (floppy_count < MAX_SPARE_DRIVES) {
				copy_path(prefs->dfxlist[floppy_count], MAX_DPATH, path_string);
				// RP9 priorities are one-based insertion preferences (priority 1
				// means DF0). Images beyond the configured drives remain available
				// in the disk swap list.
				const auto preferred_drive = media.has_priority
					? static_cast<std::uint64_t>(media.priority > 0 ? media.priority - 1 : 0)
					: static_cast<std::uint64_t>(floppy_count);
				if (preferred_drive < floppy_drive_assigned.size()
					&& preferred_drive < static_cast<std::uint64_t>(prefs->nr_floppies)
					&& !floppy_drive_assigned[preferred_drive]) {
					copy_path(prefs->floppyslots[preferred_drive].df, MAX_DPATH, path_string);
					prefs->floppyslots[preferred_drive].forcedwriteprotect = media.readonly;
					floppy_drive_assigned[preferred_drive] = true;
				}
				++floppy_count;
			}
			break;
		case rp9::MediaType::HardDrive: {
			const auto hardfile_path = prepare_undoable_hardfile(media, path, extraction_directory,
				device_number);
			if (hardfile_path.empty())
				return false;
			if (!add_harddrive(prefs, hardfile_path, media.volume_name, media.readonly, 0, device_number)) {
				set_error("Could not attach RP9 hard drive: " + media.path);
				return false;
			}
			break;
		}
		case rp9::MediaType::Cd:
			cd_paths.emplace_back(path_string);
			if (!cd_attached) {
				copy_path(prefs->cdslots[0].name, MAX_DPATH, path_string);
				prefs->cdslots[0].inuse = true;
				prefs->cdslots[0].type = SCSI_UNIT_DEFAULT;
				cd_attached = true;
			}
			break;
		case rp9::MediaType::Tape: {
			uaedev_config_info config {};
			config.type = UAEDEV_TAPE;
			config.readonly = media.readonly;
			config.blocksize = 512;
			copy_path(config.rootdir, MAX_DPATH, path_string);
			if (!add_filesys_config(prefs, -1, &config)) {
				set_error("Could not attach RP9 tape: " + media.path);
				return false;
			}
			break;
		}
		case rp9::MediaType::Snapshot:
			if (!snapshot_attached) {
				snapshot_path = path_string;
				snapshot_attached = true;
			}
			break;
		}
	}
	return true;
}

void publish_cd_swap_list(const std::vector<std::string>& cd_paths)
{
	// Keep every package disc available in the existing CD selector. Remove
	// duplicates first, then insert in reverse so priority 1 remains first.
	for (const auto& path : cd_paths) {
		lstMRUCDList.erase(std::remove(lstMRUCDList.begin(), lstMRUCDList.end(), path), lstMRUCDList.end());
	}
	for (auto path = cd_paths.rbegin(); path != cd_paths.rend(); ++path)
		add_file_to_mru_list(lstMRUCDList, *path);
	for (std::size_t index = 0; index < cd_paths.size(); ++index) {
		write_log(_T("RP9: CD swap entry %u: %s\n"), static_cast<unsigned>(index + 1), cd_paths[index].c_str());
	}
}
}

void rp9_init()
{
	loaded_path.clear();
	last_error.clear();
	loaded_snapshot_path.clear();
	loaded_has_clip = false;
	loaded_floppy_paths.clear();
	loaded_cd_paths.clear();
}

void rp9_cleanup()
{
	std::error_code error;
	for (auto it = temporary_directories.rbegin(); it != temporary_directories.rend(); ++it) {
		std::filesystem::remove_all(*it, error);
		error.clear();
	}
	temporary_directories.clear();
	rp9_init();
}

void rp9_cleanup_unused()
{
	cleanup_unused_temporary_directories();
}

bool rp9_register_rom_override(const char* filename)
{
	if (!filename || !filename[0])
		return false;
	if (getromdatabypath(filename))
		return true;

	// Cloanto-encrypted ROMs need the sibling key before their identity can be
	// checked. This is harmless for plain ROMs and mirrors normal ROM scanning.
	const auto key_path = std::filesystem::path(filename).parent_path() / "rom.key";
	std::error_code error;
	if (std::filesystem::is_regular_file(key_path, error) && !error)
		addkeyfile(key_path.string().c_str());

	auto* file = zfile_fopen(filename, _T("rb"), ZFD_NORMAL);
	if (!file)
		return false;
	auto* const rom = scan_single_rom_file(file);
	zfile_fclose(file);
	if (!rom)
		return false;

	romlist_add(filename, rom);
	return true;
}

int rp9_register_rom_directory(const char* directory)
{
	if (!directory || !directory[0])
		return 0;

	std::error_code error;
	const std::filesystem::path root(directory);
	if (!std::filesystem::is_directory(root, error) || error)
		return 0;

	std::vector<std::filesystem::path> candidates;
	std::vector<std::filesystem::path> key_files;
	constexpr auto options = std::filesystem::directory_options::skip_permission_denied;
	for (std::filesystem::recursive_directory_iterator entry(root, options, error), end;
		entry != end; entry.increment(error)) {
		if (error)
			break;

		std::error_code type_error;
		if (entry->is_directory(type_error)) {
			const auto name = entry->path().filename().string();
			if (!name.empty() && name.front() == '.')
				entry.disable_recursion_pending();
			continue;
		}
		if (type_error || !entry->is_regular_file(type_error) || type_error)
			continue;

		std::error_code size_error;
		const auto size = entry->file_size(size_error);
		if (size_error || size >= maximum_scanned_rom_size)
			continue;

		if (_tcsicmp(entry->path().filename().string().c_str(), _T("rom.key")) == 0)
			key_files.push_back(entry->path());
		else
			candidates.push_back(entry->path());
	}
	if (error) {
		write_log(_T("RP9: stopped scanning ROM directory '%s': %s\n"),
			root.string().c_str(), error.message().c_str());
	}

	std::sort(key_files.begin(), key_files.end());
	for (const auto& key_file : key_files)
		addkeyfile(key_file.string().c_str());
	std::sort(candidates.begin(), candidates.end());
	int registered = 0;
	for (const auto& candidate : candidates) {
		if (rp9_register_rom_override(candidate.string().c_str()))
			++registered;
	}
	return registered;
}

bool rp9_parse_file(uae_prefs* prefs, const char* filename)
{
	last_error.clear();
	if (!prefs || !filename || !filename[0]) {
		set_error("No RP9 package was specified");
		return false;
	}

	auto file = zfile_fopen(filename, _T("rb"));
	if (!file) {
		set_error("Could not open RP9 package: " + std::string(filename));
		return false;
	}
	auto archive = unzOpen(file);
	if (!archive) {
		zfile_fclose(file);
		set_error("RP9 package is not a valid ZIP archive: " + std::string(filename));
		return false;
	}

	std::vector<char> manifest_data;
	rp9::Manifest manifest;
	std::string manifest_error;
	bool result = read_manifest(archive, manifest_data)
		&& rp9::parse_manifest(manifest_data.data(), manifest_data.size() - 1, manifest, manifest_error);
	std::string package_deployment_id;
	if (result) {
		result = deployment_id(archive, manifest_data.data(), manifest_data.size() - 1, manifest,
			package_deployment_id);
	}
	const auto existing_deployments = result
		? find_existing_deployments(manifest, package_deployment_id)
		: std::unordered_map<std::string, std::filesystem::path> {};
	if (!result && last_error.empty())
		set_error(manifest_error);

	std::filesystem::path extraction_directory;
	auto extracted_files = existing_deployments;
	if (result) {
		extraction_directory = create_extraction_directory();
		if (extraction_directory.empty()) {
			set_error("Could not create the RP9 temporary directory");
			result = false;
		} else {
			result = extract_archive(archive, extraction_directory, extracted_files, existing_deployments);
		}
	}
	unzClose(archive);
	zfile_fclose(file);

	std::unique_ptr<uae_prefs> candidate;
	std::string snapshot_path;
	std::vector<std::string> floppy_paths;
	std::vector<std::string> cd_paths;
	if (result) {
		candidate = std::make_unique<uae_prefs>();
		result = set_default_system(candidate.get(), manifest);
	}
	if (result) {
		apply_compatibility(candidate.get(), manifest);
		apply_ram(candidate.get(), manifest);
		result = apply_peripherals(candidate.get(), manifest);
		if (result) {
			apply_video_and_clip(candidate.get(), manifest);
			result = apply_media(candidate.get(), manifest, extracted_files, package_deployment_id,
				extraction_directory, snapshot_path, floppy_paths, cd_paths);
		}
#ifdef JIT
		const bool jit_requested = manifest_requests_jit(manifest);
		if (jit_requested) {
			// An explicit JIT requirement takes precedence over cycle-exact
			// defaults inherited from the selected canonical machine profile.
			candidate->cpu_cycle_exact = false;
			candidate->cpu_memory_cycle_exact = false;
			candidate->blitter_cycle_exact = false;
		}
#else
		constexpr bool jit_requested = false;
#endif
		if (candidate->m68k_speed >= 0 && !jit_requested)
			candidate->cachesize = 0;
	}

	if (!result) {
		if (!extraction_directory.empty()) {
			std::error_code error;
			std::filesystem::remove_all(extraction_directory, error);
		}
		if (candidate)
			reset_inputdevice_config(candidate.get(), false);
		return false;
	}

	// Publish the complete configuration and package-owned global state only
	// after every manifest requirement and media attachment has succeeded.
	discard_prefs(prefs, 0);
	reset_inputdevice_config(prefs, true);
	copy_prefs(candidate.get(), prefs);
	reset_inputdevice_config(candidate.get(), false);
	savestate_state = 0;
	savestate_fname[0] = 0;
	rp9_clear_loaded_path();
	if (!snapshot_path.empty()) {
		copy_path(savestate_fname, MAX_DPATH, snapshot_path);
		savestate_state = STATE_DORESTORE;
		loaded_snapshot_path = savestate_fname;
	}

	for (const auto& warning : manifest.warnings)
		write_log(_T("RP9: %s\n"), warning.c_str());
	loaded_floppy_paths = floppy_paths;
	loaded_cd_paths = cd_paths;
	temporary_directories.emplace_back(std::move(extraction_directory));
	cleanup_unused_temporary_directories(prefs);
	publish_cd_swap_list(cd_paths);
	whdload_prefs.write_cache = false;
	whdload_prefs.whdload_filename.clear();
	loaded_path = filename;
	loaded_has_clip = manifest.has_clip;
	if (manifest.title.empty())
		write_log(_T("RP9: loaded '%s'\n"), filename);
	else
		write_log(_T("RP9: loaded '%s' (%s)\n"), filename, manifest.title.c_str());
	return true;
}

const std::string& rp9_get_loaded_path()
{
	return loaded_path;
}

const std::string& rp9_get_last_error()
{
	return last_error;
}

const std::vector<std::string>& rp9_get_loaded_floppy_paths()
{
	return loaded_floppy_paths;
}

const std::vector<std::string>& rp9_get_loaded_cd_paths()
{
	return loaded_cd_paths;
}

bool rp9_loaded_has_clip()
{
	return loaded_has_clip;
}

void rp9_clear_loaded_path()
{
	if (pending_snapshot_belongs_to_loaded_rp9()) {
		savestate_state = 0;
		savestate_fname[0] = 0;
	}
	loaded_snapshot_path.clear();
	loaded_path.clear();
	loaded_has_clip = false;
	loaded_floppy_paths.clear();
	loaded_cd_paths.clear();
}
