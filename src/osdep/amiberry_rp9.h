#pragma once

#include <string>
#include <vector>

struct uae_prefs;

void rp9_init();
void rp9_cleanup();
void rp9_cleanup_unused();
bool rp9_register_rom_override(const char* filename);
int rp9_register_rom_directory(const char* directory);
bool rp9_parse_file(struct uae_prefs* prefs, const char* filename);
const std::string& rp9_get_loaded_path();
const std::string& rp9_get_last_error();
const std::vector<std::string>& rp9_get_loaded_floppy_paths();
const std::vector<std::string>& rp9_get_loaded_cd_paths();
bool rp9_loaded_has_clip();
void rp9_clear_loaded_path();
