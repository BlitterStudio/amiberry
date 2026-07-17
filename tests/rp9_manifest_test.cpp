#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "rp9_manifest.h"

namespace
{
int failures;

void expect(const bool condition, const char* message)
{
	if (!condition) {
		std::cerr << message << '\n';
		++failures;
	}
}

bool parse(const char* xml, rp9::Manifest& manifest, std::string& error)
{
	return rp9::parse_manifest(xml, std::strlen(xml), manifest, error);
}

void test_current_namespaced_manifest()
{
	constexpr auto xml = R"xml(<?xml version="1.0"?>
<rp9 xmlns="http://www.retroplatform.com">
  <requirements><host>AmigaForever</host><playerversion>10.0.3.0</playerversion></requirements>
  <application>
    <description><title>Standards Test</title><systemrom>3.1</systemrom></description>
    <configuration>
      <system>a-1200</system><video>PAL</video>
      <ram type="chip">2097152</ram>
      <peripheral unit="1">input-joystick</peripheral>
      <compatibility>turbo-floppy</compatibility>
      <clip left="112" top="0" width="1280" height="512"/>
    </configuration>
    <media>
      <floppy root="embedded" readonly="true">Disks/Disk 1.adf</floppy>
      <floppy root="embedded">Disks/Disk 2.adf</floppy>
      <cd root="deploy">CD/game.cue</cd>
    </media>
  </application>
</rp9>)xml";

	rp9::Manifest manifest;
	std::string error;
	expect(parse(xml, manifest, error), error.c_str());
	expect(manifest.host == "AmigaForever", "host must be preserved");
	expect(manifest.system == "a-1200", "system must be normalized");
	expect(manifest.system_rom.empty(), "description/systemrom must not override playback configuration");
	expect(manifest.ram.size() == 1 && manifest.ram[0].size == 2097152, "RAM size must be parsed exactly");
	expect(manifest.peripherals.size() == 1 && manifest.peripherals[0].unit == 1,
		"peripheral unit must be parsed");
	expect(manifest.has_clip && manifest.clip.width == 1280, "clip must be parsed");
	expect(manifest.media.size() == 3, "all media entries must be preserved");
	expect(manifest.media[0].path == "Disks/Disk 1.adf" && manifest.media[1].path == "Disks/Disk 2.adf",
		"floppy manifest order must be preserved");
	expect(manifest.media[0].readonly, "media readonly attribute must be parsed");
}

void test_prefixed_namespace()
{
	constexpr auto xml = R"xml(<rp:rp9 xmlns:rp="http://www.retroplatform.com">
  <rp:requirements><rp:host>RetroPlatform</rp:host><rp:playerversion>2.2.0.0</rp:playerversion></rp:requirements>
  <rp:application><rp:description><rp:title>Prefix</rp:title></rp:description>
    <rp:configuration><rp:system>a-500</rp:system></rp:configuration><rp:media/>
  </rp:application>
</rp:rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(parse(xml, manifest, error), "prefixed RP9 namespaces must be accepted");
	expect(manifest.system == "a-500", "prefixed system element must be parsed");
}

void test_real_world_22_manifest_shape()
{
	const char* xml = "\xEF\xBB\xBF" R"xml(<?xml version="1.0" encoding="utf-8"?>
<rp9 xmlns="http://www.retroplatform.com">
  <requirements><host>Cloanto RetroPlatform Player</host><playerversion>2.2.0.0</playerversion></requirements>
  <application user-edited="true">
    <description><title>Real-world Sample</title><systemrom>130</systemrom></description>
    <configuration>
      <system>a-500</system><rom type="system">130</rom><ram type="fast">1048576</ram>
      <compatibility>flexible-blitter-cycles</compatibility><compatibility>turbo-cpu</compatibility>
      <video>ntsc</video><peripheral speed="max">cpu</peripheral>
    </configuration>
    <media><floppy>Sample.adf</floppy></media>
  </application>
</rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(parse(xml, manifest, error), "UTF-8 BOM and real-world 2.2 manifests must be accepted");
	expect(manifest.system_rom == "130", "configuration ROM must override the description value");
	expect(manifest.peripherals.size() == 1 && manifest.peripherals[0].type.empty()
		&& manifest.peripherals[0].speed == "max", "speed-only CPU peripherals must be preserved");
	expect(manifest.compatibility.size() == 2, "real-world compatibility hints must be preserved");
}

void test_real_world_30_harddrive_attributes()
{
	constexpr auto xml = R"xml(<rp9 xmlns="http://www.retroplatform.com">
  <requirements><host>RetroPlatform</host><playerversion>3.0.0.0</playerversion></requirements>
  <application><description><title>Hard Drives</title></description><configuration>
    <system>a-4000</system><peripheral layout="en-gb">keyboard</peripheral>
    <compatibility>mouseintegration</compatibility><compatibility>hostio</compatibility>
  </configuration><media>
    <harddrive priority="1" type="file" undo="false" deploy="true">disk-1.hdf</harddrive>
    <harddrive priority="2" type="file" undo="true" readonly="true">disk-2.hdf</harddrive>
  </media></application>
</rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(parse(xml, manifest, error), "real-world RP9 3.0 hard-drive attributes must be accepted");
	expect(manifest.peripherals.size() == 1 && manifest.peripherals[0].layout == "en-gb",
		"keyboard layout must be preserved");
	expect(manifest.media.size() == 2, "hard-drive media must be preserved");
	expect(manifest.media[0].has_priority && manifest.media[0].priority == 1
		&& manifest.media[0].format == "file", "hard-drive priority and type must be parsed");
	expect(manifest.media[0].has_undo && !manifest.media[0].undo && manifest.media[0].deploy,
		"hard-drive undo and deploy attributes must be parsed");
	expect(manifest.media[1].undo && manifest.media[1].readonly,
		"true hard-drive boolean attributes must be parsed");
}

void test_empty_shared_root_media()
{
	constexpr auto xml = R"xml(<rp9><requirements><host>RetroPlatform</host><playerversion>10.0.3.0</playerversion></requirements>
  <application><configuration><system>a-4000</system></configuration><media>
    <harddrive root="shared" volumename="Shared" priority="1"/>
  </media></application></rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(parse(xml, manifest, error), "an empty shared-root mount must be accepted");
	expect(manifest.media.size() == 1 && manifest.media[0].path.empty()
		&& manifest.media[0].root == "shared", "empty shared-root media must be preserved");
}

void test_medialess_package()
{
	constexpr auto xml = R"xml(<rp9><requirements><host>RetroPlatform</host><playerversion>2.2.0.0</playerversion></requirements>
  <application><description><title>Medialess</title></description>
    <configuration><system>a-500</system></configuration>
  </application></rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(parse(xml, manifest, error), "medialess RP9 packages must be accepted");
	expect(manifest.media.empty(), "medialess RP9 must not synthesize media");
}

void test_future_version_is_blocked()
{
	constexpr auto xml = R"xml(<rp9><requirements><host>RetroPlatform</host><playerversion>10.0.4.0</playerversion></requirements>
  <application><configuration><system>a-500</system></configuration></application></rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(!parse(xml, manifest, error), "future RP9 player requirements must be rejected");
	expect(error.find("newer player version") != std::string::npos, "future-version error must be actionable");
}

void test_missing_requirements_are_blocked()
{
	constexpr auto xml = R"xml(<rp9><application><configuration><system>a-500</system></configuration></application></rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(!parse(xml, manifest, error), "missing RP9 host/player requirements must be rejected");
}

void test_invalid_numbers_are_blocked()
{
	constexpr auto invalid_ram = R"xml(<rp9><requirements><host>RetroPlatform</host><playerversion>2.2.0.0</playerversion></requirements>
  <application><configuration><system>a-500</system><ram type="chip">2MB</ram></configuration></application></rp9>)xml";
	constexpr auto invalid_clip = R"xml(<rp9><requirements><host>RetroPlatform</host><playerversion>2.2.0.0</playerversion></requirements>
  <application><configuration><system>a-500</system><clip left="112" top="0" width="0" height="512"/></configuration></application></rp9>)xml";
	rp9::Manifest manifest;
	std::string error;
	expect(!parse(invalid_ram, manifest, error), "non-numeric RP9 RAM must be rejected");
	expect(!parse(invalid_clip, manifest, error), "zero-sized RP9 clips must be rejected");
}

void test_package_paths_are_normalized_safely()
{
	std::string normalized;
	expect(rp9::normalize_package_path("Disks\\Disk 1.adf", normalized)
		&& normalized == "Disks/Disk 1.adf", "package paths must normalize separators");
	expect(rp9::normalize_package_path("Disks/./Disk 2.adf", normalized)
		&& normalized == "Disks/Disk 2.adf", "package paths must normalize dot components");
	expect(!rp9::normalize_package_path("../escape.adf", normalized), "parent traversal must be rejected");
	expect(!rp9::normalize_package_path("Disks/../../escape.adf", normalized), "nested traversal must be rejected");
	expect(!rp9::normalize_package_path("Disks/../escape.adf", normalized),
		"parent components must be rejected even when normalization would stay inside the package");
	expect(!rp9::normalize_package_path("/absolute.adf", normalized), "absolute POSIX paths must be rejected");
	expect(!rp9::normalize_package_path("C:\\absolute.adf", normalized), "absolute Windows paths must be rejected");
	expect(!rp9::normalize_package_path("Disks/data:stream.adf", normalized),
		"Windows alternate-data-stream paths must be rejected");
	expect(!rp9::normalize_package_path("Disks/NUL.adf", normalized),
		"Windows device names must be rejected on every host");
	expect(!rp9::normalize_package_path("Disks/trailing. ", normalized),
		"non-portable trailing dots and spaces must be rejected");
}
}

int main(const int argc, char** argv)
{
	if (argc > 1) {
		for (int index = 1; index < argc; ++index) {
			std::ifstream file;
			std::istream* input = &std::cin;
			if (std::strcmp(argv[index], "-") != 0) {
				file.open(argv[index], std::ios::binary);
				input = &file;
			}
			const std::string xml((std::istreambuf_iterator<char>(*input)), std::istreambuf_iterator<char>());
			rp9::Manifest manifest;
			std::string error;
			if (!*input || !rp9::parse_manifest(xml.data(), xml.size(), manifest, error)) {
				std::cerr << argv[index] << ": " << (*input ? error : "could not read manifest") << '\n';
				++failures;
			}
		}
		return failures == 0 ? 0 : 1;
	}

	test_current_namespaced_manifest();
	test_prefixed_namespace();
	test_real_world_22_manifest_shape();
	test_real_world_30_harddrive_attributes();
	test_empty_shared_root_media();
	test_medialess_package();
	test_future_version_is_blocked();
	test_missing_requirements_are_blocked();
	test_invalid_numbers_are_blocked();
	test_package_paths_are_normalized_safely();
	return failures == 0 ? 0 : 1;
}
