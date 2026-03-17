#include "sysdeps.h"
#include "file_dialog.h"

#include <cfloat>
#include <cctype>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ImGuiFileDialog.h"
#include <imgui.h>

#include "amiberry_gfx.h"
#include "macos_bookmarks.h"
#include "options.h"
#include "xwin.h"

#ifdef HAS_NFD
#include <nfd.h>
#include "nfd_sdl3.h"
#endif

extern void write_log(const char* format, ...);

namespace
{
	std::unordered_map<std::string, std::string> s_results;

	static const char* dialog_key_or_default(const char* key)
	{
		return key ? key : "";
	}

#ifdef HAS_NFD
	bool s_nfd_initialized = false;
	bool s_nfd_available = false;
	bool s_wayland_display_set = false;

	struct ParsedNFDFilters
	{
		std::vector<std::string> names;
		std::vector<std::string> specs;
		std::vector<nfdu8filteritem_t> items;
	};

	static std::string trim_copy(const std::string& in)
	{
		size_t start = 0;
		while (start < in.size() && std::isspace(static_cast<unsigned char>(in[start])))
			++start;

		size_t end = in.size();
		while (end > start && std::isspace(static_cast<unsigned char>(in[end - 1])))
			--end;

		return in.substr(start, end - start);
	}

	static std::vector<std::string> split_top_level(const std::string& input, const char delimiter)
	{
		std::vector<std::string> parts;
		std::string current;
		int brace_depth = 0;
		int paren_depth = 0;

		for (const char ch : input)
		{
			if (ch == '{')
				++brace_depth;
			else if (ch == '}' && brace_depth > 0)
				--brace_depth;
			else if (ch == '(')
				++paren_depth;
			else if (ch == ')' && paren_depth > 0)
				--paren_depth;

			if (ch == delimiter && brace_depth == 0 && paren_depth == 0)
			{
				parts.push_back(trim_copy(current));
				current.clear();
				continue;
			}

			current.push_back(ch);
		}

		if (!current.empty())
			parts.push_back(trim_copy(current));

		return parts;
	}

	static std::string normalize_extension(std::string ext)
	{
		ext = trim_copy(ext);
		if (ext.empty())
			return {};

		while (!ext.empty() && ext[0] == '*')
			ext.erase(0, 1);
		while (!ext.empty() && ext[0] == '.')
			ext.erase(0, 1);

		if (ext.empty() || ext == "*")
			return {};

		return ext;
	}

	static void add_filter_group(ParsedNFDFilters& parsed, const std::string& name, const std::vector<std::string>& exts)
	{
		std::string spec;
		for (const auto& ext : exts)
		{
			const std::string normalized = normalize_extension(ext);
			if (normalized.empty())
				continue;

			if (!spec.empty())
				spec += ',';
			spec += normalized;
		}

		if (spec.empty())
			return;

		parsed.names.push_back(name.empty() ? "Files" : name);
		parsed.specs.push_back(std::move(spec));
	}

	static ParsedNFDFilters parse_igfd_filters(const char* filters)
	{
		ParsedNFDFilters parsed;

		if (!filters)
			return parsed;

		const std::string all = trim_copy(filters);
		if (all.empty() || all == "*" || all == ".*")
			return parsed;

		if (all.find('{') == std::string::npos || all.find('}') == std::string::npos)
		{
			add_filter_group(parsed, "Files", split_top_level(all, ','));
		}
		else
		{
			const auto groups = split_top_level(all, ',');
			for (const auto& group : groups)
			{
				if (group.empty())
					continue;

				const size_t open = group.find('{');
				const size_t close = group.rfind('}');
				if (open == std::string::npos || close == std::string::npos || close <= open)
					continue;

				const std::string name = trim_copy(group.substr(0, open));
				const std::string spec = trim_copy(group.substr(open + 1, close - open - 1));
				if (spec.empty() || spec == "*" || spec == ".*")
					continue;

				add_filter_group(parsed, name.empty() ? "Files" : name, split_top_level(spec, ','));
			}
		}

		parsed.items.reserve(parsed.specs.size());
		for (size_t i = 0; i < parsed.specs.size(); ++i)
		{
			nfdu8filteritem_t item{};
			item.name = parsed.names[i].c_str();
			item.spec = parsed.specs[i].c_str();
			parsed.items.push_back(item);
		}

		return parsed;
	}
#endif

#ifdef HAS_NFD
	static void ensure_wayland_display()
	{
		if (s_wayland_display_set)
			return;

		AmigaMonitor* mon = &AMonitors[0];
		if (mon->gui_window)
		{
			NFD_SetDisplayPropertiesFromSDL3Window(mon->gui_window);
			s_wayland_display_set = true;
		}
	}
#endif

	static bool can_use_native_dialog()
	{
		if (kmsdrm_detected)
			return false;

		if (currprefs.headless)
			return false;

#ifdef HAS_NFD
		if (!s_nfd_initialized)
			return false;

		if (!s_nfd_available)
			return false;

		return true;
#else
		return false;
#endif
	}

	static void open_igfd_file_dialog(const char* key, const char* title, const char* filters, const std::string& initialPath)
	{
		IGFD::FileDialogConfig config;
		config.path = initialPath;
		config.countSelectionMax = 1;
		config.flags = ImGuiFileDialogFlags_Modal;
		ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
	}

	static void open_igfd_dir_dialog(const char* key, const std::string& initialPath)
	{
		IGFD::FileDialogConfig config;
		config.path = initialPath;
		config.countSelectionMax = 1;
		config.flags = ImGuiFileDialogFlags_Modal;
		ImGuiFileDialog::Instance()->OpenDialog(key, "Choose Directory", nullptr, config);
	}

	static bool ConsumeIGFDResult(const char* dialogKey, std::string& outPath,
		const std::function<std::string(ImGuiFileDialog*)>& getResult)
	{
		ImVec2 minSize(0, 0), maxSize(FLT_MAX, FLT_MAX);
		ImGuiViewport* vp = ImGui::GetMainViewport();
		if (vp)
		{
			const float vw = vp->Size.x;
			const float vh = vp->Size.y;
			minSize = ImVec2(vw * 0.50f, vh * 0.50f);
			maxSize = ImVec2(vw * 0.95f, vh * 0.90f);
			ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(vw * 0.70f, vh * 0.70f), ImGuiCond_Appearing);
		}

		if (!ImGuiFileDialog::Instance()->Display(dialogKey, ImGuiWindowFlags_NoCollapse, minSize, maxSize))
			return false;

		bool ok = false;
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			outPath = getResult(ImGuiFileDialog::Instance());
			macos_bookmark_store(outPath);
			ok = true;
		}
		ImGuiFileDialog::Instance()->Close();
		return ok;
	}
}

void file_dialog_init()
{
#ifdef HAS_NFD
	if (s_nfd_initialized)
		return;

	s_nfd_initialized = true;
	s_nfd_available = false;

	if (NFD_Init() != NFD_OKAY)
	{
		write_log("NFD_Init failed: %s\n", NFD_GetError());
		return;
	}

	s_nfd_available = true;
	AmigaMonitor* mon = &AMonitors[0];
	if (mon->gui_window)
	{
		if (!NFD_SetDisplayPropertiesFromSDL3Window(mon->gui_window))
			write_log("NFD_SetDisplayPropertiesFromSDL3Window failed\n");
	}
#endif
}

void file_dialog_shutdown()
{
#ifdef HAS_NFD
	if (s_nfd_initialized && s_nfd_available)
		NFD_Quit();

	s_nfd_initialized = false;
	s_nfd_available = false;
	s_wayland_display_set = false;
#endif
	s_results.clear();
}

void OpenFileDialogKey(const char* key, const char* title, const char* filters, const std::string& initialPath)
{
	key = dialog_key_or_default(key);
	title = title ? title : "Choose File";
	filters = filters ? filters : ".*";

	if (can_use_native_dialog())
	{
#ifdef HAS_NFD
		ensure_wayland_display();
		ParsedNFDFilters parsed = parse_igfd_filters(filters);

		nfdopendialogu8args_t args{};
		if (!parsed.items.empty())
		{
			args.filterList = parsed.items.data();
			args.filterCount = static_cast<nfdfiltersize_t>(parsed.items.size());
		}
		args.defaultPath = initialPath.empty() ? nullptr : initialPath.c_str();

		AmigaMonitor* mon = &AMonitors[0];
		if (mon->gui_window)
			NFD_GetNativeWindowFromSDL3Window(mon->gui_window, &args.parentWindow);

		nfdu8char_t* outPath = nullptr;
		const nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
		if (result == NFD_OKAY)
		{
			std::string path = outPath ? outPath : "";
			if (outPath)
				NFD_FreePathU8(outPath);
			s_results[key] = path;
			if (!path.empty())
				macos_bookmark_store(path);
			return;
		}

		if (result == NFD_CANCEL)
		{
			s_results[key] = "";
			return;
		}

		write_log("NFD_OpenDialogU8_With failed: %s — falling back to built-in file browser\n", NFD_GetError());
		s_nfd_available = false;  // don't retry NFD on subsequent calls
		if (outPath)
			NFD_FreePathU8(outPath);
#endif
	}

	open_igfd_file_dialog(key, title, filters, initialPath);
}

bool ConsumeFileDialogResultKey(const char* key, std::string& outPath)
{
	key = dialog_key_or_default(key);

	const auto it = s_results.find(key);
	if (it != s_results.end())
	{
		outPath = it->second;
		s_results.erase(it);
		return !outPath.empty();
	}

	return ConsumeIGFDResult(key, outPath,
		[](ImGuiFileDialog* dlg) { return dlg->GetFilePathName(); });
}

bool IsFileDialogOpenKey(const char* key)
{
	key = dialog_key_or_default(key);

	if (s_results.find(key) != s_results.end())
		return false;

	return ImGuiFileDialog::Instance()->IsOpened(key);
}

void OpenDirDialogKey(const char* key, const std::string& initialPath)
{
	key = dialog_key_or_default(key);

	if (can_use_native_dialog())
	{
#ifdef HAS_NFD
		ensure_wayland_display();
		nfdpickfolderu8args_t args{};
		args.defaultPath = initialPath.empty() ? nullptr : initialPath.c_str();

		AmigaMonitor* mon = &AMonitors[0];
		if (mon->gui_window)
			NFD_GetNativeWindowFromSDL3Window(mon->gui_window, &args.parentWindow);

		nfdu8char_t* outPath = nullptr;
		const nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
		if (result == NFD_OKAY)
		{
			std::string path = outPath ? outPath : "";
			if (outPath)
				NFD_FreePathU8(outPath);
			s_results[key] = path;
			if (!path.empty())
				macos_bookmark_store(path);
			return;
		}

		if (result == NFD_CANCEL)
		{
			s_results[key] = "";
			return;
		}

		write_log("NFD_PickFolderU8_With failed: %s — falling back to built-in file browser\n", NFD_GetError());
		s_nfd_available = false;
		if (outPath)
			NFD_FreePathU8(outPath);
#endif
	}

	open_igfd_dir_dialog(key, initialPath);
}

bool ConsumeDirDialogResultKey(const char* key, std::string& outPath)
{
	key = dialog_key_or_default(key);

	const auto it = s_results.find(key);
	if (it != s_results.end())
	{
		outPath = it->second;
		s_results.erase(it);
		return !outPath.empty();
	}

	return ConsumeIGFDResult(key, outPath,
		[](ImGuiFileDialog* dlg) { return dlg->GetCurrentPath(); });
}

bool IsDirDialogOpenKey(const char* key)
{
	key = dialog_key_or_default(key);

	if (s_results.find(key) != s_results.end())
		return false;

	return ImGuiFileDialog::Instance()->IsOpened(key);
}

void OpenFileDialog(const char* title, const char* filters, const std::string& initialPath)
{
	OpenFileDialogKey("ChooseFileDlgKey", title, filters, initialPath);
}

bool ConsumeFileDialogResult(std::string& outPath)
{
	return ConsumeFileDialogResultKey("ChooseFileDlgKey", outPath);
}

bool IsFileDialogOpen()
{
	return IsFileDialogOpenKey("ChooseFileDlgKey");
}

void OpenDirDialog(const std::string& initialPath)
{
	OpenDirDialogKey("ChooseDirDlgKey", initialPath);
}

bool ConsumeDirDialogResult(std::string& outPath)
{
	return ConsumeDirDialogResultKey("ChooseDirDlgKey", outPath);
}

bool IsDirDialogOpen()
{
	return IsDirDialogOpenKey("ChooseDirDlgKey");
}
