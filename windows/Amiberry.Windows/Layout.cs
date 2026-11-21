namespace Amiberry.Windows;

internal sealed record SettingSection(string Page, string Title, string[] Ids, bool Advanced = false, string Kind = "options");

internal static class Layout
{
    private static SettingSection S(string page, string title, string ids, bool advanced = false) => new(page, title, ids.Split(' ', StringSplitOptions.RemoveEmptyEntries), advanced);
    private static SettingSection Special(string page, string title, string kind) => new(page, title, [], false, kind);
    public static readonly Dictionary<string, string[]> Groups = new()
    {
        ["hardware"] = ["cpu", "mmu", "fpu", "cpu_speed", "jit", "chipset", "adv_chipset", "memory", "rom", "accelerator", "rtg", "expansions", "ppc", "x86", "genlock"],
        ["display"] = ["display", "filter", "sound"], ["input"] = ["input", "onscreen"],
        ["storage"] = ["floppy", "diskswapper", "cd", "filesystem", "drawbridge"], ["ports"] = ["io", "misc", "priority"]
    };
    public static readonly SettingSection[] Sections =
    [
        S("hardware", "CPU", "cpu_model cpu_compatible cpu_24bit_addressing cpu_speed fpu_model"),
        S("hardware", "CPU · Advanced", "cpu_throttle cpu_multiplier amiberry.cpu_idle cpu_threaded cachesize compfpu comp_constjump comp_flushmode comp_nf comp_catchfault comp_trustbyte cpu_no_unimplemented cpu_data_cache mmu_model fpu_strict fpu_softfloat fpu_no_unimplemented", true),
        S("hardware", "Chipset", "chipset chipset_compatible ntsc cycle_exact immediate_blits collision_level"),
        S("hardware", "Chipset · Advanced", "waiting_blits amiberry.multithreaded_denise keyboard_type keyboard_nkro display_optimizations hvcsync rtc chipset_rtc_adjust ciaatod cia_overlay a1000ram df0idhw cd32cd cd32c2p cd32nvram cdtvcd cdtvram cdtv-cr ksmirror_e0 ksmirror_a8 color_burst resetwarning cia_todbug cs_bytecustomwritebug memory_pattern rom_is_slow toshiba_gary z3_autoconfig 1mchipjumper ciaa_type unmapped_address_space eclocksync ramsey fatgary agnusrev deniserev agnusmodel agnussize denisemodel", true),
        S("hardware", "Memory", "chipmem_size bogomem_size fastmem_size z3mem_size fastmem2_size fastmem2_size_k fastmem3_size fastmem3_size_k fastmem4_size fastmem4_size_k z3mem2_size z3mem3_size z3mem4_size"),
        S("hardware", "Memory · Advanced", "fastmem_size_k z3mapping a3000mem_size mbresmem_size megachipmem_size", true),
        S("hardware", "ROMs", "kickstart_rom_file kickstart_ext_rom_file cart_file"),
        S("hardware", "ROMs · Advanced", "boot_rom_uae uaeboard flash_file rtc_file maprom kickshifter", true),
        S("hardware", "Accelerator board", "cpuboard_type cpuboardmem1_size cpuboard_rom_file cpuboard_settings"),
        S("hardware", "RTG board", "gfxcard_type gfxcard_size gfxcard_options:monitor gfxcard_options:autoswitch gfxcard_options:initial gfxcard_hardware_sprite"),
        S("hardware", "RTG · Advanced", "gfxcard_options rtg_modes rtg_modes:8bit rtg_modes:16bit rtg_modes:24bit rtg_modes:32bit gfx_filter_autoscale_rtg gfx_filter_autoscale_limit_rtg gfx_backbuffers_rtg gfxcard_zerocopy gfxcard_multithread gfxcard_hardware_vblank amiberry.rtg_scale_allow amiberry.rtg_scale_aspect_ratio amiberry.rtg_vblank zz9000_net zz9000_int2 gfx_display_rtg", true),
        S("hardware", "Expansions", "bsdsocket_emu scsi sana2"),
        Special("hardware", "Expansion Boards", "boards"),
        S("hardware", "On-board interfaces", "pcmcia ide scsi_a3000 scsi_a4000t"),
        S("hardware", "PowerPC", "ppc_model ppc_implementation ppc_cpu_idle", true),
        S("hardware", "x86 Bridgeboard", "cpu_x86_throttle", true),
        S("hardware", "Genlock", "genlock genlockmode genlock_image genlock_video genlock_mix genlock_alpha genlock_aspect", true),
        S("display", "Window & Full Screen", "gfx_display gfx_fullscreen_amiga gfx_fullscreen_picasso gfx_width_windowed gfx_height_windowed gfx_resize_windowed amiberry.borderless"),
        S("display", "Picture", "gfx_resolution gfx_overscanmode gfx_autoresolution gfx_autoresolution_vga amiberry.gfx_correct_aspect amiberry.scaling_method gfx_linemode gfx_lores_mode gfx_center_horizontal gfx_center_vertical gfx_ntscpixelspal"),
        S("display", "Crop & Position", "amiberry.gfx_auto_crop amiberry.gfx_manual_crop amiberry.gfx_manual_crop_width amiberry.gfx_manual_crop_height amiberry.gfx_horizontal_offset amiberry.gfx_vertical_offset"),
        S("display", "Shaders & Bezels", "amiberry.shader amiberry.shader_rtg amiberry.use_bezel amiberry.use_custom_bezel amiberry.custom_bezel"),
        S("display", "Colour", "gfx_luminance gfx_contrast gfx_gamma gfx_gamma_r gfx_gamma_g gfx_gamma_b gfx_blacker_than_black gfx_monochrome gfx_atari_palette_fix gfx_flickerfixer"),
        S("display", "Sync", "gfx_vsync gfx_vsync_picasso gfx_vrr_monitor"),
        S("display", "Sync · Advanced", "gfx_monitorblankdelay displaydata_pal displaydata_ntsc", true),
        S("display", "Sound Output", "amiberry.soundcard_default amiberry.soundcard sound_output sound_auto sound_frequency sound_max_buff"),
        S("display", "Volume", "sound_volume sound_volume_paula sound_volume_cd sound_volume_ahi sound_volume_midi"),
        S("display", "Stereo & Filter", "sound_channels sound_stereo_separation sound_stereo_mixing_delay sound_stereo_swap_paula sound_stereo_swap_ahi sound_interpol sound_filter sound_filter_type"),
        new("display", "Drive Sounds", Enumerable.Range(0, 4).SelectMany(i => new[] { $"floppy{i}sound", $"floppy{i}soundvolume_disk", $"floppy{i}soundvolume_empty" }).ToArray()),
        S("input", "Port 1", "joyport0 joyport0mode joyport0autofire joyport0mousemap"),
        S("input", "Port 2", "joyport1 joyport1mode joyport1autofire joyport1mousemap"),
        S("input", "Parallel Port Adapter", "joyport2 joyport2autofire joyport3 joyport3autofire"),
        S("input", "Mouse", "input.mouse_speed input.autoswitch input.multi_mouse magic_mouse amiberry.middle_mouse magic_mousecursor tablet_library absolute_mouse"),
        S("input", "Joystick", "input.joystick_deadzone input.autofire_speed input.joystick_up_button input.joymouse_speed_digital input.joymouse_speed_analog"),
        Special("input", "Custom Controls", "controls"),
        S("input", "Keyboard Shortcuts", "amiberry.open_gui amiberry.quit_amiberry amiberry.action_replay amiberry.fullscreen_toggle amiberry.minimize amiberry.screenshot amiberry.debugger_trigger amiberry.left_amiga amiberry.right_amiga"),
        S("input", "Controller Shortcuts", "amiberry.use_retroarch_menu amiberry.use_retroarch_quit amiberry.use_retroarch_reset"),
        S("input", "On-screen Controls", "amiberry.onscreen_joystick amiberry.vkbd_enabled amiberry.vkbd_language amiberry.vkbd_numpad amiberry.vkbd_transparency amiberry.vkbd_touch_button amiberry.vkbd_toggle amiberry.use_retroarch_vkbd"),
        Special("storage", "Hard Drives", "mounts"),
        S("storage", "Floppy Drives", "floppy_speed"),
        S("storage", "DF0:", "floppy0type floppy0subtype floppy0"), S("storage", "DF1:", "floppy1type floppy1subtype floppy1"),
        S("storage", "DF2:", "floppy2type floppy2subtype floppy2"), S("storage", "DF3:", "floppy3type floppy3subtype floppy3"),
        S("storage", "CD", "cdimage0 cd_speed amiberry.map_cd_drives"),
        new("storage", "Disk Swapper", Enumerable.Range(0, 20).Select(i => $"diskimage{i}").ToArray(), true),
        S("storage", "File System", "filesys_custom_uaefsdb filesys_no_fsdb", true),
        S("storage", "DrawBridge", "amiberry.drawbridge_driver amiberry.drawbridge_drive_cable amiberry.drawbridge_serial_autodetect amiberry.drawbridge_serial_port amiberry.drawbridge_smartspeed amiberry.drawbridge_autocache", true),
        S("ports", "Printer & Sampler", "amiberry.parallel_port amiberry.parallel_port:folder amiberry.samplersoundcard sampler_stereo"),
        S("ports", "Serial & MIDI", "serial_port amiberry.midiout_device_name amiberry.midiin_device_name"),
        S("ports", "Serial & MIDI · Advanced", "serial_on_demand serial_hardware_ctsrts serial_status serial_ri serial_direct serial_translate uaeserial amiberry.midirouter", true),
        S("ports", "Copy Protection", "dongle"), S("ports", "Startup", "use_gui cpu_reset_pause warpboot"),
        S("ports", "Status Line", "show_leds show_leds_rtg power_led_dim"), S("ports", "Host Integration", "clipboard_sharing synchronize_clock"),
        S("ports", "Window", "amiberry.start_minimized amiberry.always_on_top amiberry.inactive_minimize amiberry.blank_monitors"),
        S("ports", "Mouse & Keyboard", "amiberry.start_not_captured amiberry.active_capture_automatically amiberry.ctrl_alt_release amiberry.alt_tab_release amiberry.right_control_is_right_win amiberry.kbd_led_num amiberry.kbd_led_scr amiberry.kbd_led_cap"),
        S("ports", "Safety & Debugging", "floppy_write_protect harddrive_write_protect cpu_halt_auto_reset native_code log_illegal_mem debug_mem", true),
        S("ports", "Compatibility", "uae_hide_autoconfig scsidevice_disable", true), S("ports", "Priority", "", true)
    ];
}
