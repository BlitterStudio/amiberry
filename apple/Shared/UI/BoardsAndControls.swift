/*
 * Copyright (C) 2026 BlitterStudio
 * The editor's expansion boards (Hardware) and custom controls (Input):
 * the parts of a machine that are not options but units and mappings,
 * edited through the session's board and custom-control calls.
 */

import SwiftUI

// MARK: Expansion boards

/// The expansion board units the machine has, and adding more.
struct BoardsSections: View {
	let store: SettingsStore

	/// The board types under a category heading: each type under the first
	/// category it has, in this order.
	static let categories: [(title: String, category: BoardType.Categories)] = [
		("Built-in", .builtIn), ("SCSI", .scsi), ("IDE", .ide), ("SASI", .sasi), ("Network", .network),
		("Sound", .sound), ("Graphics", .rtg), ("Floppy", .floppy), ("PCI bridges", .pciBridge),
		("x86 bridgeboards", .x86Bridge), ("x86 cards", .x86Expansion), ("Other", .custom),
	]

	static func category(of type: BoardType) -> String {
		categories.first { type.categories.contains($0.category) }?.title ?? "Other"
	}

	struct Unit: Identifiable {
		let type: BoardType
		let unit: Int
		var id: String { "\(type.id)#\(unit)" }
	}

	var body: some View {
		let units = enabledUnits()
		Section {
			if units.isEmpty {
				Text("No expansion boards.").foregroundStyle(.secondary)
			}
			ForEach(units) { unit in
				BoardUnitView(type: unit.type, unit: unit.unit, store: store)
			}
			Menu("Add Board") {
				ForEach(BoardsSections.categories, id: \.title) { heading in
					let types = BoardCatalog.types.filter { BoardsSections.category(of: $0) == heading.title }
					if !types.isEmpty {
						Menu(heading.title) {
							ForEach(types) { type in
								Button(type.manufacturer.map { "\(type.label) (\($0))" } ?? type.label) { add(type) }
									.disabled(freeUnit(of: type) == nil)
							}
						}
					}
				}
			}
			.accessibilityIdentifier("boards.add")
		} header: {
			Text("Expansion Boards")
		} footer: {
			Text("Boards are built when the emulation starts.")
		}
	}

	private func enabledUnits() -> [Unit] {
		BoardCatalog.types.flatMap { type in
			(0..<type.units).compactMap { unit in
				store.boardValue("enabled", type: type.id, unit: unit) == "true" ? Unit(type: type, unit: unit) : nil
			}
		}
	}

	/// The type's first unit not in use, when the machine can have it (a
	/// built-in controller only on the machines that have it).
	private func freeUnit(of type: BoardType) -> Int? {
		guard let unit = (0..<type.units).first(where: { store.boardValue("enabled", type: type.id, unit: $0) != "true" }),
			store.isBoardFieldAvailable("enabled", type: type.id, unit: unit)
		else { return nil }
		return unit
	}

	private func add(_ type: BoardType) {
		guard let unit = freeUnit(of: type) else { return }
		store.setBoardValue("true", field: "enabled", type: type.id, unit: unit)
	}
}

/// One board unit: its fields, and removing it.
struct BoardUnitView: View {
	let type: BoardType
	let unit: Int
	let store: SettingsStore
	@Environment(SettingsWindowModel.self) private var model: SettingsWindowModel?

	var body: some View {
		DisclosureGroup {
			ForEach(type.fields.filter { $0.id != "enabled" }) { field in
				BoardFieldRow(field: field, type: type, unit: unit, store: store)
			}
			// A built-in controller stays while the machine has it.
			Button("Remove Board", role: .destructive) {
				if store.setBoardValue("false", field: "enabled", type: type.id, unit: unit) {
					// Text refused in its fields goes with the board.
					let prefix = "board:\(type.id):\(unit):"
					model?.invalidDrafts = (model?.invalidDrafts ?? [:]).filter { !$0.key.hasPrefix(prefix) }
				}
			}
			.disabled(!store.isBoardFieldAvailable("enabled", type: type.id, unit: unit))
		} label: {
			VStack(alignment: .leading, spacing: 2) {
				Text(type.units > 1 ? "\(type.label) #\(unit + 1)" : type.label)
				if let manufacturer = type.manufacturer {
					Text(manufacturer).font(.caption).foregroundStyle(.secondary)
				}
			}
		}
	}
}

/// A field of a board unit, with the control its type calls for.
struct BoardFieldRow: View {
	let field: SettingDescriptor
	let type: BoardType
	let unit: Int
	let store: SettingsStore
	@Environment(SettingsWindowModel.self) private var model: SettingsWindowModel?
	@State private var draft = ""

	/// The key text the layer refused is kept under, so the window does not
	/// close over it (as for option text fields).
	private var draftKey: String { "board:\(type.id):\(unit):\(field.id)" }

	var body: some View {
		let available = store.isBoardFieldAvailable(field.id, type: type.id, unit: unit)
		control
			.disabled(!available)
			.help(field.help)
	}

	private var value: String { store.boardValue(field.id, type: type.id, unit: unit) ?? "" }

	@discardableResult
	private func set(_ newValue: String) -> Bool {
		store.setBoardValue(newValue, field: field.id, type: type.id, unit: unit)
	}

	@ViewBuilder private var control: some View {
		switch field.kind {
		case .bool:
			Toggle(field.label, isOn: Binding(get: { value == "true" }, set: { set($0 ? "true" : "false") }))
		case .choice:
			let current = value
			let choices = field.choices.contains { $0.value == current } || current.isEmpty
				? field.choices : [SettingChoice(value: current, label: current)] + field.choices
			Picker(field.label, selection: Binding(get: { current }, set: { set($0) })) {
				ForEach(choices) { Text($0.label).tag($0.value) }
			}
		case .integer, .real:
			let range = field.range ?? 0...0
			let number = Double(value) ?? range.lowerBound
			Stepper(value: Binding(get: { number }, set: { set(field.kind == .integer ? String(Int($0)) : String($0)) }),
				in: range, step: field.step ?? 1) {
				LabeledContent(field.label, value: value)
			}
		case .path, .string:
			LabeledContent(field.label) {
				HStack(spacing: 6) {
					TextField(field.label, text: $draft)
						.labelsHidden()
					if field.kind == .path {
						Button("Choose\u{2026}") {
							if let path = FilePanels.choose(.rom, title: field.label, startingAt: draft) {
								draft = path
							}
						}
					}
				}
			}
			.onAppear { draft = model?.invalidDrafts[draftKey] ?? value }
			.onChange(of: store.revision) {
				if draft != value, model?.invalidDrafts[draftKey] == nil {
					draft = value
				}
			}
			// Stored as it is typed, as option text fields are; text the layer
			// refuses stays in the field until it is corrected.
			.onChange(of: draft) {
				if draft == value || set(draft) {
					model?.invalidDrafts[draftKey] = nil
				} else {
					model?.invalidDrafts[draftKey] = draft
				}
			}
		}
	}
}

// MARK: Custom controls

/// What each button and axis of a port's joystick triggers, alone or with
/// the device's hotkey held.
struct CustomControlsSections: View {
	let store: SettingsStore
	@State private var port = 1
	@State private var hotkey = false

	static let ports = [(0, "Mouse port"), (1, "Joystick port"), (2, "Parallel port 1"), (3, "Parallel port 2")]

	var body: some View {
		Section {
			Picker("Port", selection: $port) {
				ForEach(CustomControlsSections.ports, id: \.0) { Text($0.1).tag($0.0) }
			}
			Picker("Mapping", selection: $hotkey) {
				Text("Alone").tag(false)
				Text("With the hotkey held").tag(true)
			}
			if store.customControl("a", port: port, hotkey: hotkey) == nil {
				Text("No joystick is connected to this port.").foregroundStyle(.secondary)
			} else {
				ForEach(CustomControlCatalog.controls) { control in
					CustomControlRow(control: control, port: port, hotkey: hotkey, store: store)
				}
			}
		} header: {
			Text("Custom Controls")
		} footer: {
			Text("Default leaves a control to what the port's mode gives it; None makes it do nothing.")
		}
	}
}

struct CustomControlRow: View {
	let control: CustomControl
	let port: Int
	let hotkey: Bool
	let store: SettingsStore

	var body: some View {
		let value = store.customControl(control.id, port: port, hotkey: hotkey) ?? ""
		if let use = store.customControlUse(control.id, port: port, hotkey: hotkey) {
			LabeledContent(control.label, value: "Used by \(CustomControlRow.useTitle(use))")
		} else {
			Picker(control.label, selection: Binding(get: { value }, set: {
				store.setCustomControl($0, control: control.id, port: port, hotkey: hotkey)
			})) {
				Text("Default").tag("")
				Text("None").tag("none")
				ForEach(CustomControlCatalog.events, id: \.self) { Text($0).tag($0) }
			}
			.disabled(!store.isCustomControlAvailable(control.id, port: port, hotkey: hotkey))
		}
	}

	static func useTitle(_ use: String) -> String {
		switch use {
		case "hotkey": return "the hotkey"
		case "quit": return "Quit"
		case "menu": return "the menu"
		case "reset": return "Reset"
		case "vkbd": return "the on-screen keyboard"
		default: return use
		}
	}
}
