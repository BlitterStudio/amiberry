using System.Text.Json.Nodes;

namespace Amiberry.Windows;

public sealed partial class MainWindow
{
    private static IEnumerable<(string Title, IEnumerable<JsonNode> Options, bool Collapsed)> AppSections(string group, IEnumerable<JsonNode> settings)
    {
        var options = settings.ToArray();
        (string Title, string Ids, bool Collapsed)[] sections = group switch
        {
            "global_input" =>
            [
                ("Controllers & Mouse", "default_controller1 default_controller2 default_controller3 default_controller4 default_mouse1 default_mouse2 input_default_mouse_speed default_joystick_deadzone input_keyboard_as_joystick_stop_keypresses rctrl_as_ramiga", false),
                ("Default Shortcuts", "default_open_gui_key default_quit_key default_ar_key default_fullscreen_toggle_key default_screenshot_key", false),
                ("RetroArch", "default_retroarch_quit default_retroarch_menu default_retroarch_reset", true)
            ],
            "global_display" =>
            [
                ("Display Defaults", "default_line_mode default_horizontal_centering default_vertical_centering default_scaling_method default_gfx_autoresolution default_correct_aspect_ratio default_auto_crop default_multithreaded_denise", false),
                ("Window Defaults", "default_width default_height default_fullscreen_mode", false)
            ],
            "global_paths" => [("Logging", "write_logfile logfile_path", false)],
            _ => []
        };
        if (sections.Length == 0) { yield return (HostGroupTitle(group), options, group == "global_advanced"); yield break; }
        var placed = new HashSet<string>();
        foreach (var section in sections)
        {
            var ids = section.Ids.Split(' ').ToHashSet();
            var selected = options.Where(option => ids.Contains(option.S("id"))).ToArray();
            foreach (var option in selected) placed.Add(option.S("id"));
            if (selected.Length > 0) yield return (section.Title, selected, section.Collapsed);
        }
        var remaining = options.Where(option => !placed.Contains(option.S("id"))).ToArray();
        if (remaining.Length > 0) yield return (group == "global_paths" ? "Folders" : "More", remaining, false);
    }
}
