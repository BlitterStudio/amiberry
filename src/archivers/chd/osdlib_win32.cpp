// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont
//============================================================
//
//  osdlib_win32.cpp - Windows OSD library functions
//
//  Provides the platform-specific implementations for:
//  - Environment variables
//  - Process control
//  - Clipboard access (via SDL2)
//  - Dynamic module loading (via LoadLibrary)
//  - Virtual memory allocation (via VirtualAlloc)
//  - Instruction cache invalidation
//
//============================================================

// MAME headers
#include "osdcore.h"
#include "osdlib.h"

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


//============================================================
//  osd_getenv
//============================================================

const char* osd_getenv(const char* name)
{
	return getenv(name);
}

//============================================================
//  osd_setenv
//============================================================

int osd_setenv(const char* name, const char* value, int overwrite)
{
	// MinGW provides _putenv_s; use SetEnvironmentVariable for overwrite semantics
	if (!overwrite)
	{
		const char* existing = getenv(name);
		if (existing)
			return 0;
	}
	// _putenv_s returns 0 on success
	char buf[4096];
	std::snprintf(buf, sizeof(buf), "%s=%s", name, value);
	return _putenv(buf);
}

//============================================================
//  osd_process_kill
//============================================================

void osd_process_kill()
{
	TerminateProcess(GetCurrentProcess(), 1);
}

//============================================================
//  osd_break_into_debugger
//============================================================

void osd_break_into_debugger(const char* message)
{
#ifdef MAME_DEBUG
	printf("MAME exception: %s\n", message);
	printf("Attempting to fall into debugger\n");
	if (IsDebuggerPresent())
		DebugBreak();
#else
	printf("Ignoring MAME exception: %s\n", message);
#endif
}


//============================================================
//  osd_get_clipboard_text
//============================================================

std::string osd_get_clipboard_text()
{
	std::string result;

	if (SDL_HasClipboardText())
	{
		char* temp = SDL_GetClipboardText();
		result.assign(temp);
		SDL_free(temp);
	}
	return result;
}


//============================================================
//  osd_getpid
//============================================================

int osd_getpid()
{
	return static_cast<int>(GetCurrentProcessId());
}


namespace osd {

	namespace {

		class dynamic_module_win32_impl : public dynamic_module
		{
		public:
			dynamic_module_win32_impl(std::vector<std::string>&& libraries) : m_libraries(std::move(libraries))
			{
			}

			virtual ~dynamic_module_win32_impl() override
			{
				if (m_module)
					FreeLibrary(m_module);
			}

		protected:
			virtual generic_fptr_t get_symbol_address(char const* symbol) override
			{
				/*
				 * given a list of libraries, if a first symbol is successfully loaded from
				 * one of them, all additional symbols will be loaded from the same library
				 */
				if (m_module)
					return reinterpret_cast<generic_fptr_t>(GetProcAddress(m_module, symbol));

				for (auto const& library : m_libraries)
				{
					HMODULE const module = LoadLibraryA(library.c_str());

					if (module != nullptr)
					{
						generic_fptr_t const function = reinterpret_cast<generic_fptr_t>(GetProcAddress(module, symbol));

						if (function)
						{
							m_module = module;
							return function;
						}
						else
						{
							FreeLibrary(module);
						}
					}
				}

				return nullptr;
			}

		private:
			std::vector<std::string> m_libraries;
			HMODULE m_module = nullptr;
		};

	} // anonymous namespace


	bool invalidate_instruction_cache(void const* start, std::size_t size)
	{
		return FlushInstructionCache(GetCurrentProcess(), start, size) != 0;
	}


	void* virtual_memory_allocation::do_alloc(std::initializer_list<std::size_t> blocks, unsigned intent, std::size_t& size, std::size_t& page_size)
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		std::size_t const p = info.dwPageSize;
		if (0 == p)
			return nullptr;

		std::size_t s(0);
		for (std::size_t b : blocks)
			s += (b + p - 1) / p;
		s *= p;
		if (!s)
			return nullptr;

		void* const result = VirtualAlloc(nullptr, s, MEM_COMMIT | MEM_RESERVE, PAGE_NOACCESS);
		if (!result)
			return nullptr;

		size = s;
		page_size = p;
		return result;
	}

	void virtual_memory_allocation::do_free(void* start, std::size_t size)
	{
		VirtualFree(start, 0, MEM_RELEASE);
	}

	bool virtual_memory_allocation::do_set_access(void* start, std::size_t size, unsigned access)
	{
		DWORD prot;
		if (access == NONE)
			prot = PAGE_NOACCESS;
		else if (access == READ)
			prot = PAGE_READONLY;
		else if (access == (READ | WRITE))
			prot = PAGE_READWRITE;
		else if (access == (READ | EXECUTE))
			prot = PAGE_EXECUTE_READ;
		else if (access == (READ | WRITE | EXECUTE))
			prot = PAGE_EXECUTE_READWRITE;
		else if (access == EXECUTE)
			prot = PAGE_EXECUTE;
		else
			prot = PAGE_NOACCESS;

		DWORD old_prot;
		return VirtualProtect(start, size, prot, &old_prot) != 0;
	}


	dynamic_module::ptr dynamic_module::open(std::vector<std::string>&& names)
	{
		return std::make_unique<dynamic_module_win32_impl>(std::move(names));
	}

} // namespace osd
