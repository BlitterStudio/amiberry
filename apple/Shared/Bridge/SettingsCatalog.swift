/*
 * Copyright (C) 2026 BlitterStudio
 * The settings layer's static data (src/osdep/settings/amiberry_settings.h)
 * as Swift values: option descriptors, machine presets and the errors the
 * layer reports. Descriptors and presets are static, so these types can be
 * used from any thread.
 */

import AmiberrySettings

/// When a committed change reaches the running emulation.
public enum ApplyClass: Int, Comparable, Sendable, CaseIterable {
	/// Picked up by the running emulation.
	case live
	/// Takes effect at the next hard reset.
	case reset
	/// Takes effect the next time emulation starts.
	case restart

	init(_ value: amiberry_apply_class) {
		switch value {
		case AMIBERRY_APPLY_RESET: self = .reset
		case AMIBERRY_APPLY_RESTART: self = .restart
		default: self = .live
		}
	}

	var layerValue: amiberry_apply_class {
		switch self {
		case .live: return AMIBERRY_APPLY_LIVE
		case .reset: return AMIBERRY_APPLY_RESET
		case .restart: return AMIBERRY_APPLY_RESTART
		}
	}

	public static func < (lhs: ApplyClass, rhs: ApplyClass) -> Bool {
		lhs.rawValue < rhs.rawValue
	}
}

/// One value of a choice option: `value` in .uae spelling, `label` to show.
public struct SettingChoice: Hashable, Sendable, Identifiable {
	public let value: String
	public let label: String
	public var id: String { value }

	public init(value: String, label: String) {
		self.value = value
		self.label = label
	}
}

/// An option of the settings layer. `id` is its .uae configuration key and
/// every value uses the .uae spelling.
public struct SettingDescriptor: Hashable, Sendable, Identifiable {
	public enum Kind: Hashable, Sendable {
		/// "true" or "false".
		case bool
		/// A decimal integer within `range`.
		case integer
		/// A decimal number within `range`.
		case real
		/// One of `choices`, or of the host's devices (see `SettingsSession.choices(for:)`).
		case choice
		/// A file system path a front end can attach a file picker to.
		case path
		/// Any other literal value, for example a hexadecimal address.
		case string
		/// A keyboard shortcut: "" (none), a key with modifiers, a modifier
		/// key alone or a controller button (see `HotkeyCatalog`).
		case hotkey
	}

	public let id: String
	/// A stable group ID, for example "cpu" or "jit".
	public let group: String
	public private(set) var label: String
	public let help: String
	public let kind: Kind
	public let applyClass: ApplyClass
	/// The widest range of an integer or real option; the range it accepts
	/// now can be narrower (`SettingsSession.range(for:)`).
	public let range: ClosedRange<Double>?
	public let step: Double?
	public let defaultValue: String
	/// The descriptor's own choices; empty for options that list the host's
	/// devices.
	public let choices: [SettingChoice]
	/// Why the emulator may change a value the user picked.
	public let constraint: String?
	/// An integer counting the other way from how it reads (a volume stored
	/// as its attenuation): shown as `shown(_:)` of the stored value.
	public let reversed: Bool
	/// The unit of an integer or real option's shown number ("%", "KB",
	/// "px"), written after it; nil for none. The label does not repeat it.
	public let unit: String?
	/// What a stored number is divided by to show it: 10 for a gamma stored
	/// in tenths (125 shows as 12.5), 1 for a number shown as stored.
	public let scale: Double
	/// A level people set by feel (a volume, the brightness, a dead zone)
	/// rather than a number they look up: shown as a slider.
	public let level: Bool

	/// The number a person sees for a stored one: a reversed option's
	/// range turned around (100 - 0 = full volume), then divided by the
	/// scale.
	public func shown(_ number: Double) -> Double {
		var number = number
		if reversed, let range {
			number = range.lowerBound + range.upperBound - number
		}
		return number / scale
	}

	/// A range of stored numbers as the numbers shown.
	public func shown(_ range: ClosedRange<Double>) -> ClosedRange<Double> {
		let ends = [shown(range.lowerBound), shown(range.upperBound)]
		return ends.min()!...ends.max()!
	}

	/// The stored number for a shown one, `shown(_:)` undone and rounded to
	/// the option's step (counted from the bottom of its range) and, for an
	/// integer, to a whole number.
	public func stored(fromShown number: Double) -> Double {
		var number = number * scale
		if reversed, let range {
			number = range.lowerBound + range.upperBound - number
		}
		if let step, step > 0 {
			let origin = range?.lowerBound ?? 0
			number = origin + ((number - origin) / step).rounded() * step
		}
		return kind == .integer ? number.rounded() : number
	}

	/// Whether the option is an on/off switch stored as "1" and "0" (see
	/// `isSwitch(kind:range:choices:)`).
	public var isSwitch: Bool {
		SettingDescriptor.isSwitch(kind: kind, range: range, choices: choices)
	}

	/// Whether a value shows as a switch: the option is one, and the value
	/// is "0" or "1". A configuration can hold another (an auto-resolution
	/// threshold the GUI copied in), which a switch would show as off and
	/// lose when turned on; that one keeps the option's other control.
	public func showsAsSwitch(_ value: String) -> Bool {
		isSwitch && (value == "0" || value == "1")
	}

	/// The labels that make a first choice of "0" read as off.
	private static let offLabels: Set<String> = ["disabled", "off", "no", "none"]

	/// Whether an option reads as an on/off switch, off stored as "0" and
	/// on as "1": an integer from 0 to 1, or exactly two choices, "0" then
	/// "1", the first named as off ("Disabled", "Off", "No", "None").
	public static func isSwitch(kind: Kind, range: ClosedRange<Double>?, choices: [SettingChoice]) -> Bool {
		switch kind {
		case .integer:
			return range == 0...1
		case .choice:
			return choices.map(\.value) == ["0", "1"] && offLabels.contains(choices[0].label.lowercased())
		default:
			return false
		}
	}

	/// The same option under another label: a designed page names it in
	/// the words of its section ("Image" under "DF0:").
	public func labelled(_ text: String) -> SettingDescriptor {
		var copy = self
		copy.label = text
		return copy
	}

	init(_ d: amiberry_setting_descriptor) {
		id = layerString(d.id)
		group = layerString(d.group)
		label = layerString(d.label)
		help = layerString(d.help)
		switch d.type {
		case AMIBERRY_SETTING_BOOL: kind = .bool
		case AMIBERRY_SETTING_INT: kind = .integer
		case AMIBERRY_SETTING_REAL: kind = .real
		case AMIBERRY_SETTING_CHOICE: kind = .choice
		case AMIBERRY_SETTING_PATH: kind = .path
		case AMIBERRY_SETTING_HOTKEY: kind = .hotkey
		default: kind = .string
		}
		applyClass = ApplyClass(d.apply)
		if kind == .integer || kind == .real {
			range = d.min <= d.max ? d.min...d.max : nil
			step = d.step
		} else {
			range = nil
			step = nil
		}
		defaultValue = layerString(d.default_value)
		var choices: [SettingChoice] = []
		if let list = d.choices {
			for i in 0..<d.choice_count {
				choices.append(SettingChoice(value: layerString(list[i].value), label: layerString(list[i].label)))
			}
		}
		self.choices = choices
		constraint = optionalLayerString(d.constraint)
		reversed = kind == .integer && d.reversed
		let number = kind == .integer || kind == .real
		unit = number ? optionalLayerString(d.unit) : nil
		scale = number && d.scale != 0 ? d.scale : 1
		level = number && d.level
	}
}

/// Every option of the settings layer, in the layer's order.
public enum SettingsCatalog {
	/// The version of amiberry_settings.h this module was built against.
	public static let apiVersion = Int(AMIBERRY_SETTINGS_API_VERSION)

	public static let descriptors: [SettingDescriptor] = {
		(0..<amiberry_settings_count()).compactMap { index in
			amiberry_settings_at(index).map { SettingDescriptor($0.pointee) }
		}
	}()

	private static let byID: [String: SettingDescriptor] = Dictionary(
		descriptors.map { ($0.id, $0) }, uniquingKeysWith: { first, _ in first })

	public static func descriptor(for id: String) -> SettingDescriptor? {
		byID[id]
	}

	/// The group IDs in the order their first option appears.
	public static let groups: [String] = {
		var seen = Set<String>()
		return descriptors.compactMap { seen.insert($0.group).inserted ? $0.group : nil }
	}()

	public static func descriptors(in group: String) -> [SettingDescriptor] {
		descriptors.filter { $0.group == group }
	}
}

/// A machine preset (a Quickstart model) and its configurations.
public struct MachinePreset: Hashable, Sendable, Identifiable {
	/// A stable ID, for example "a500".
	public let id: String
	public let label: String
	/// The configurations' labels; a configuration is its index.
	public let configurations: [String]
	/// The highest compatibility level (an index into `MachinePresets.compatibilityLevels`).
	public let compatibilityMax: Int
}

public enum MachinePresets {
	/// The most common models, listed first wherever models are listed (the
	/// rest follow in the layer's order).
	public static let commonIDs = ["a500", "a1200", "cd32", "a4000"]

	public static let all: [MachinePreset] = {
		let presets: [MachinePreset] = (0..<amiberry_settings_preset_count()).compactMap { index in
			guard let preset = amiberry_settings_preset_at(index) else { return nil }
			let configurations = (0..<Int(preset.pointee.config_count)).map {
				layerString(amiberry_settings_preset_config_label(preset, Int32($0)))
			}
			return MachinePreset(
				id: layerString(preset.pointee.id), label: layerString(preset.pointee.label),
				configurations: configurations, compatibilityMax: Int(preset.pointee.compatibility_max))
		}
		let common = commonIDs.compactMap { id in presets.first { $0.id == id } }
		return common + presets.filter { !commonIDs.contains($0.id) }
	}()

	/// The compatibility levels' labels, from the most compatible (0).
	public static let compatibilityLevels: [String] = {
		var levels: [String] = []
		while let label = amiberry_settings_preset_compatibility_label(Int32(levels.count)) {
			levels.append(String(cString: label))
		}
		return levels
	}()

	public static func preset(for id: String) -> MachinePreset? {
		all.first { $0.id == id }
	}

	/// The compatibility level a preset is applied with by default, as the
	/// Quickstart panel and --apply-preset do: 1, or the preset's highest
	/// when that is lower.
	public static func defaultCompatibility(for id: String) -> Int {
		min(1, preset(for: id)?.compatibilityMax ?? 0)
	}

	/// The computers, which boot a Kickstart (not the CD consoles or the
	/// arcade boards).
	public static let computerIDs: Set<String> = ["a500", "a500plus", "a600", "a1000", "a1200", "a3000", "a4000", "a4000t"]

	/// Whether some computer has every ROM of one of its configurations:
	/// false until a Kickstart is installed (machines then boot the
	/// built-in AROS ROM).
	@MainActor
	public static func anyKickstartInstalled() -> Bool {
		all.filter { computerIDs.contains($0.id) }.contains { preset in
			preset.configurations.indices.contains { isInstalled(preset, configuration: $0) }
		}
	}

	/// Whether any configuration of the preset has every ROM it needs.
	@MainActor
	public static func isRunnable(_ preset: MachinePreset) -> Bool {
		preset.configurations.indices.contains { isInstalled(preset, configuration: $0) }
	}

	/// Whether every ROM the configuration needs is in the emulator's ROM
	/// list; never shows a dialog or changes anything.
	@MainActor
	public static func isInstalled(_ preset: MachinePreset, configuration: Int) -> Bool {
		amiberry_settings_preset_rom_installed(preset.id, Int32(clamping: configuration))
	}
}

/// What the settings layer refused.
public enum SettingsError: Error, Hashable, Sendable {
	/// No option, preset or entry has this ID or index.
	case unknownID(String)
	/// The value cannot be parsed, is out of range or is not a choice.
	case invalidValue(id: String, value: String)
	/// The option, this value or this request is not available now.
	case unavailable(String)
	/// The value does not fit any buffer the bridge offers.
	case valueTooLong(String)
	/// The layer could not allocate memory.
	case outOfMemory

	/// nil for AMIBERRY_SETTINGS_OK.
	init?(_ status: amiberry_settings_status, id: String, value: String = "") {
		switch status {
		case AMIBERRY_SETTINGS_OK: return nil
		case AMIBERRY_SETTINGS_UNKNOWN_ID: self = .unknownID(id)
		case AMIBERRY_SETTINGS_INVALID_VALUE: self = .invalidValue(id: id, value: value)
		case AMIBERRY_SETTINGS_UNAVAILABLE: self = .unavailable(id)
		default: self = .valueTooLong(id)
		}
	}

	static func check(_ status: amiberry_settings_status, id: String, value: String = "") throws {
		if let error = SettingsError(status, id: id, value: value) {
			throw error
		}
	}
}

/// An expansion board type: SCSI, IDE and network controllers, bridgeboards,
/// sound and graphics cards and the machines' built-in controllers. A unit
/// of it has `fields`: "enabled" first, then as the type has them
/// "subtype", "rom_file", "scsi_id", "autoboot_disabled", "inserted",
/// "dma24bit" and the board's own "setting:<name>" fields.
public struct BoardType: Hashable, Sendable, Identifiable {
	public struct Categories: OptionSet, Hashable, Sendable {
		public let rawValue: UInt32
		public init(rawValue: UInt32) { self.rawValue = rawValue }
		public static let builtIn = Categories(rawValue: AMIBERRY_BOARD_INTERNAL)
		public static let scsi = Categories(rawValue: AMIBERRY_BOARD_SCSI)
		public static let ide = Categories(rawValue: AMIBERRY_BOARD_IDE)
		public static let sasi = Categories(rawValue: AMIBERRY_BOARD_SASI)
		public static let custom = Categories(rawValue: AMIBERRY_BOARD_CUSTOM)
		public static let pciBridge = Categories(rawValue: AMIBERRY_BOARD_PCI_BRIDGE)
		public static let x86Bridge = Categories(rawValue: AMIBERRY_BOARD_X86_BRIDGE)
		public static let rtg = Categories(rawValue: AMIBERRY_BOARD_RTG)
		public static let sound = Categories(rawValue: AMIBERRY_BOARD_SOUND)
		public static let network = Categories(rawValue: AMIBERRY_BOARD_NETWORK)
		public static let floppy = Categories(rawValue: AMIBERRY_BOARD_FLOPPY)
		public static let x86Expansion = Categories(rawValue: AMIBERRY_BOARD_X86_EXPANSION)
	}

	/// The configuration name, for example "a2091".
	public let id: String
	public let label: String
	public let manufacturer: String?
	public let categories: Categories
	/// How many of it a configuration can have; units are numbered from 0.
	public let units: Int
	public let fields: [SettingDescriptor]

	init(_ pointer: UnsafePointer<amiberry_settings_board_type>) {
		let t = pointer.pointee
		id = layerString(t.id)
		label = layerString(t.label)
		manufacturer = optionalLayerString(t.manufacturer)
		categories = Categories(rawValue: UInt32(t.categories))
		units = Int(t.units)
		fields = (0..<t.field_count).compactMap { index in
			amiberry_settings_board_field_at(pointer, index).map { SettingDescriptor($0.pointee) }
		}
	}
}

/// Every expansion board type, in the layer's order.
public enum BoardCatalog {
	public static let types: [BoardType] = {
		(0..<amiberry_settings_board_type_count()).compactMap { index in
			amiberry_settings_board_type_at(index).map { BoardType($0) }
		}
	}()

	public static func type(_ id: String) -> BoardType? {
		types.first { $0.id == id }
	}
}

/// A gamepad button or axis of the joystick on a port, whose Amiga input
/// event a configuration sets (custom controls).
public struct CustomControl: Hashable, Sendable, Identifiable {
	/// The configuration name, for example "a", "dpup" or "leftx".
	public let id: String
	public let label: String
	public let isAxis: Bool
}

/// The custom controls and the input events they can trigger.
/// A Global Settings entry (amiberry.conf): process-wide, saved explicitly
/// with `HostSettings.save()`.
public struct HostSetting: Hashable, Sendable, Identifiable {
	public let id: String
	public let group: String
	public let label: String
	public let kind: SettingDescriptor.Kind
	public let defaultValue: String
	/// The whole descriptor: help, choices and range.
	public let descriptor: SettingDescriptor
}

/// The host settings, in the layer's order.
public enum HostSettings {
	public static let all: [HostSetting] = {
		(0..<amiberry_host_settings_count()).compactMap { index in
			amiberry_host_settings_at(index).map {
				let d = $0.pointee
				let descriptor = SettingDescriptor(d)
				return HostSetting(id: descriptor.id, group: descriptor.group,
					label: descriptor.label, kind: descriptor.kind,
					defaultValue: descriptor.defaultValue, descriptor: descriptor)
			}
		}
	}()

	@MainActor
	public static func value(of id: String) throws -> String {
		let (text, status) = readLayerString(capacity: 4096) {
			amiberry_host_settings_get(id, $0, $1)
		}
		try SettingsError.check(status, id: id)
		return text
	}

	@MainActor
	public static func set(_ value: String, for id: String) throws {
		try SettingsError.check(amiberry_host_settings_set(id, value), id: id, value: value)
	}

	/// Writes amiberry.conf. False when the file could not be written.
	@MainActor
	public static func save() -> Bool {
		amiberry_host_settings_save()
	}

	/// The choices of a choice setting: its own list, or the host's devices,
	/// shaders or bezels as the layer lists them now. A value can lie outside
	/// them (a device that is not connected).
	@MainActor
	public static func choices(for id: String) -> [SettingChoice] {
		var list: UnsafePointer<amiberry_setting_choice>?
		let count = amiberry_host_settings_choices(id, &list)
		guard let list else { return [] }
		return (0..<count).map { SettingChoice(value: layerString(list[$0].value), label: layerString(list[$0].label)) }
	}

	/// Re-reads the host's device lists, for example after a device was connected.
	@MainActor
	public static func refreshChoices() {
		amiberry_host_settings_refresh_choices()
	}
}

public enum CustomControlCatalog {
	public static let controls: [CustomControl] = {
		(0..<amiberry_settings_custom_control_count()).compactMap { index in
			amiberry_settings_custom_control_at(index).map {
				CustomControl(id: layerString($0.pointee.id), label: layerString($0.pointee.label),
					isAxis: $0.pointee.axis)
			}
		}
	}()

	/// The events by the names configurations use ("Space",
	/// "Joy2 Fire/Mouse2 Left Button"); a control can also be "none" (does
	/// nothing) or "" (does what the port's mode gives it).
	public static let events: [String] = {
		(0..<amiberry_settings_input_event_count()).compactMap { index in
			amiberry_settings_input_event_at(index).map { layerString($0.pointee.value) }
		}
	}()
}

/// The modifier keys held with a recorded key: amiberry_settings.h's
/// AMIBERRY_HOTKEY_* bits. Option is Alt and Command is GUI.
public struct HotkeyModifiers: OptionSet, Hashable, Sendable {
	public let rawValue: UInt32
	public init(rawValue: UInt32) { self.rawValue = rawValue }
	public static let leftControl = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_LCTRL)
	public static let rightControl = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_RCTRL)
	public static let leftShift = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_LSHIFT)
	public static let rightShift = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_RSHIFT)
	public static let leftOption = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_LALT)
	public static let rightOption = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_RALT)
	public static let leftCommand = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_LGUI)
	public static let rightCommand = HotkeyModifiers(rawValue: AMIBERRY_HOTKEY_RGUI)
}

/// Keyboard shortcut values (`SettingDescriptor.Kind.hotkey`): the layer
/// spells a recorded key press the way the emulator reads it, and lists the
/// controller buttons a shortcut can name.
public enum HotkeyCatalog {
	/// Value: SDL's button name ("guide"); label: the button's name.
	public static let buttons: [SettingChoice] = {
		var list: UnsafePointer<amiberry_setting_choice>?
		let count = amiberry_settings_hotkey_buttons(&list)
		guard let list else { return [] }
		return (0..<count).map { SettingChoice(value: layerString(list[$0].value), label: layerString(list[$0].label)) }
	}()

	/// Whether the emulator answers a controller button bound to this option
	/// or host setting (the Amiga key options only map a key).
	public static func takesButtons(_ id: String) -> Bool {
		amiberry_settings_hotkey_takes_buttons(id)
	}

	/// The value for a key press: `usage` is the key's USB HID keyboard
	/// usage, or 0 for a modifier key pressed alone (then `modifiers` holds
	/// exactly that key). Nil for a key a shortcut cannot name, such as a
	/// modifier usage or Keypad +.
	public static func format(usage: UInt32, modifiers: HotkeyModifiers) -> String? {
		var buffer = [CChar](repeating: 0, count: Int(AMIBERRY_SETTINGS_VALUE_MAX))
		let spelled = buffer.withUnsafeMutableBufferPointer {
			amiberry_settings_hotkey_format(usage, modifiers.rawValue, $0.baseAddress, $0.count)
		}
		return spelled ? decodedString(buffer) : nil
	}
}
