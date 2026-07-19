#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rp9
{
struct Version
{
	unsigned int major = 0;
	unsigned int minor = 0;
	unsigned int patch = 0;
	unsigned int build = 0;
};

enum class MediaType
{
	Floppy,
	HardDrive,
	Cd,
	Tape,
	Snapshot
};

struct Ram
{
	std::string type;
	std::uint64_t size = 0;
};

struct Peripheral
{
	std::string name;
	std::string type;
	std::string speed;
	std::string layout;
	std::string path;
	std::string device;
	std::string device_name;
	std::string volume_name;
	int unit = -1;
	std::uint64_t memory = 0;
	bool has_memory = false;
	bool fpu = false;
	bool has_fpu = false;
};

struct Boot
{
	std::string type;
	std::string value;
	bool readonly = false;
};

struct Clip
{
	unsigned int left = 0;
	unsigned int top = 0;
	unsigned int width = 0;
	unsigned int height = 0;
};

struct Media
{
	MediaType type = MediaType::Floppy;
	std::string path;
	std::string root;
	std::string format;
	std::string volume_name;
	unsigned int priority = 0;
	bool has_priority = false;
	bool readonly = false;
	bool undo = false;
	bool has_undo = false;
	bool deploy = false;
};

struct Manifest
{
	std::string host;
	Version player_version;
	std::string title;
	std::string system;
	std::string system_rom;
	std::string video;
	std::vector<Ram> ram;
	std::vector<Peripheral> peripherals;
	std::vector<std::string> compatibility;
	std::vector<Media> media;
	Boot boot;
	bool has_boot = false;
	Clip clip;
	bool has_clip = false;
	std::vector<std::string> warnings;
};

bool parse_version(const std::string& text, Version& version);
bool is_supported_version(const Version& version);
bool normalize_package_path(const std::string& original, std::string& normalized);
bool parse_manifest(const char* xml, std::size_t size, Manifest& manifest, std::string& error);
}
