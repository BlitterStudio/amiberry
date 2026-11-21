/* Windows native frontend's main-thread settings and actions bridge. */
#pragma once

#include <string>
#include <nlohmann/json_fwd.hpp>

// Called only on the emulator main thread. Returns the protocol data value;
// failures throw std::runtime_error for the transport's error envelope.
nlohmann::json windows_native_dispatch(const nlohmann::json& request);
void windows_native_close_session();
// Final process shutdown only: cancel and join any WHDBoot download worker.
void windows_native_shutdown_bridge();

// Core strings are UTF-8, except configuration and media paths that older
// builds saved in the ANSI code page. A string that is not valid UTF-8 is read
// as ANSI and returned as UTF-8; anything else is returned as it is. The pipe
// protocol is UTF-8.
std::string windows_native_core_to_utf8(const std::string& text);
