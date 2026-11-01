/*
 * The built-in GUI's state and helpers the rest of the emulator uses, in
 * every build: the native Apple apps build without the ImGui GUI
 * (main_window.cpp) but run the same core.
 */
#include "sysconfig.h"
#include "sysdeps.h"

#include "target.h"
#include "gui/gui_handling.h"
#include "tinyxml2.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

bool gui_running = false;
bool joystick_refresh_needed = false;

// A message from before the emulation runs (the JIT's access violations):
// the GUI shows it, at once or when it next opens.
void target_startup_msg(const TCHAR* title, const TCHAR* msg)
{
	gui_message(_T("%s: %s"), title, msg);
}

void gui_restart()
{
	gui_running = false;
}

std::string get_json_timestamp(const std::string& json_filename)
{
	std::ifstream f(json_filename);
	if (!f.is_open())
		return "";

	try
	{
		// Read only enough to find the timestamp fields near the top
		// Parse the full file since nlohmann doesn't support partial parsing easily
		nlohmann::json doc = nlohmann::json::parse(f, nullptr, false);
		f.close();
		if (doc.is_discarded())
			return "";

		// Prefer upstream_timestamp, fall back to generated
		if (doc.contains("upstream_timestamp") && doc["upstream_timestamp"].is_string())
			return doc["upstream_timestamp"].get<std::string>();
		if (doc.contains("generated") && doc["generated"].is_string())
			return doc["generated"].get<std::string>();
	}
	catch (...)
	{
		// Parse failed
	}
	return "";
}

std::string get_xml_timestamp(const std::string& xml_filename)
{
	std::string result;
	tinyxml2::XMLDocument doc;
	auto error = false;

	auto* f = fopen(xml_filename.c_str(), _T("rb"));
	if (f)
	{
		auto err = doc.LoadFile(f);
		if (err != tinyxml2::XML_SUCCESS)
		{
			write_log(_T("Failed to parse '%s':  %d\n"), xml_filename.c_str(), err);
			error = true;
		}
		fclose(f);
	}
	else
	{
		error = true;
	}
	if (!error)
	{
		auto* whdbooter = doc.FirstChildElement("whdbooter");
		result = whdbooter->Attribute("timestamp");
	}
	if (!result.empty())
		return result;
	return "";
}
