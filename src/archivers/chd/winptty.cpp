// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, Vas Crabb
//============================================================
//
//  winptty.cpp - Windows pseudo-terminal stub
//
//  PTY is not available on Windows in the same way as Unix.
//  This provides stub implementations that return "not supported".
//
//============================================================

#include "winfile.h"

#include <cstdint>
#include <string>
#include <system_error>


bool win_check_ptty_path(std::string const& path) noexcept
{
	// No PTY support on Windows
	return false;
}


std::error_condition win_open_ptty(std::uint32_t openflags, osd_file::ptr& file, std::uint64_t& filesize, std::string& name) noexcept
{
	return std::errc::not_supported;
}
