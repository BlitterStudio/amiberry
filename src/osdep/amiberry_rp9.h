#pragma once

#include <string>

struct uae_prefs;

void rp9_init();
void rp9_cleanup();
bool rp9_parse_file(struct uae_prefs* prefs, const char* filename);
const std::string& rp9_get_loaded_path();
const std::string& rp9_get_last_error();
bool rp9_loaded_has_clip();
void rp9_clear_loaded_path();
