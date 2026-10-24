/*
 * Copyright (C) 2026 BlitterStudio
 * What the overlay's menu button opens over a running emulation: the
 * drives, the keyboard and on-screen joystick, states, reset, the machine's
 * settings and the way back to the Library.
 */

import SwiftUI

struct RunningSheet: View {
	let state: OverlayState
	/// Closes the sheet; what it starts (a file picker, the settings
	/// screen) shows once it has gone.
	let close: () -> Void
	@State private var joystick = false
	@State private var confirmingStop = false
	@State private var notice: String?

	var body: some View {
		let status = state.status.status
		NavigationStack {
			Form {
				if let notice {
					Section { Text(notice).foregroundStyle(.red) }
				}
				drives(status)
				Section("Controls") {
					if state.keyboardOptions.enabled {
						Button {
							run { EmulationOverlay.shared.toggleKeyboard() }
						} label: {
							Label(state.keyboardVisible ? "Hide Keyboard" : "Show Keyboard", systemImage: "keyboard")
						}
					}
					Toggle(isOn: Binding(get: { joystick }, set: setJoystick)) {
						Label("On-screen Joystick", systemImage: "gamecontroller")
					}
				}
				Section("State") {
					action("Quick Save State", "square.and.arrow.down", .saveStateSlot(0))
					action("Quick Load State", "square.and.arrow.up", .loadStateSlot(0))
					action("Save State\u{2026}", "doc.badge.plus", .chooseMedia(.saveState))
					action("Load State\u{2026}", "doc", .chooseMedia(.loadState))
				}
				Section("Machine") {
					action(status.runState == .paused ? "Resume" : "Pause",
						status.runState == .paused ? "play" : "pause",
						status.runState == .paused ? .resume : .pause)
					action("Reset", "arrow.counterclockwise", .reset(hard: false, keyboard: false))
					action("Hard Reset", "power", .reset(hard: true, keyboard: false))
					action("Settings\u{2026}", "gearshape", .openSettings)
				}
				Section {
					Button("Quit to Library", role: .destructive) { confirmingStop = true }
						.accessibilityIdentifier("running.stop")
				}
			}
			.navigationTitle(status.configDescription.isEmpty ? "Amiga" : status.configDescription)
			.navigationBarTitleDisplayMode(.inline)
			.toolbar {
				ToolbarItem(placement: .confirmationAction) {
					Button("Done", action: close)
				}
			}
			.confirmationDialog("Quit to the Library?", isPresented: $confirmingStop, titleVisibility: .visible) {
				Button("Quit to Library", role: .destructive) { post(.stop) }
					.accessibilityIdentifier("running.stop.confirm")
			} message: {
				Text("The machine stops. Anything it has not saved is lost.")
			}
		}
		.onAppear {
			joystick = (try? SettingsSession.activeValue(of: "amiberry.onscreen_joystick")) == "true"
		}
	}

	@ViewBuilder private func drives(_ status: EmulatorStatus) -> some View {
		let drives = status.floppies.indices.filter { !status.floppies[$0].disabled }
		let swapper = (0..<EmulatorActions.swapperEntryCount).compactMap { index -> (Int, String)? in
			let path = EmulatorActions.swapperEntry(index)
			return path.isEmpty ? nil : (index, path)
		}
		Section("Drives") {
			ForEach(drives, id: \.self) { drive in
				driveRow("DF\(drive):", path: status.floppies[drive].mediaPath,
					insert: .chooseMedia(.floppy(drive: drive)), eject: .ejectFloppy(drive: drive))
			}
			if status.cdActivity != nil || !status.cdPath.isEmpty {
				driveRow("CD", path: status.cdPath, insert: .chooseMedia(.cd), eject: .ejectCD)
			}
			if !swapper.isEmpty {
				Menu {
					ForEach(swapper, id: \.0) { index, path in
						Menu((path as NSString).lastPathComponent) {
							ForEach(drives, id: \.self) { drive in
								Button("Insert into DF\(drive):") { post(.insertSwapperEntry(index: index, drive: drive)) }
							}
						}
					}
				} label: {
					Label("Disk Swapper", systemImage: "square.stack")
				}
			}
		}
	}

	private func driveRow(_ name: String, path: String, insert: EmulatorAction, eject: EmulatorAction) -> some View {
		HStack {
			Text(name).monospacedDigit().frame(width: 44, alignment: .leading)
			Text(path.isEmpty ? "Empty" : (path as NSString).lastPathComponent)
				.foregroundStyle(path.isEmpty ? .secondary : .primary)
				.lineLimit(1).truncationMode(.middle)
			Spacer()
			Button("Insert\u{2026}") { post(insert) }
				.buttonStyle(.borderless)
			Button("Eject") { post(eject) }
				.buttonStyle(.borderless)
				.disabled(path.isEmpty)
		}
	}

	private func action(_ title: String, _ symbol: String, _ action: EmulatorAction) -> some View {
		Button { post(action) } label: { Label(title, systemImage: symbol) }
	}

	/// Closes the sheet, then posts: pickers and the settings screen show
	/// over the emulation, not over the sheet.
	private func post(_ action: EmulatorAction) {
		run { ReportedActions.post(action) }
	}

	private func run(_ work: @escaping () -> Void) {
		close()
		Task { @MainActor in work() }
	}

	/// A configuration option, so the running machine is changed through a
	/// session and applied as the editor would.
	private func setJoystick(_ on: Bool) {
		do {
			let session = try SettingsSession()
			try session.setValue(on ? "true" : "false", for: "amiberry.onscreen_joystick")
			let result = try session.commit()
			if result.changed {
				ReportedActions.post(.applySettings(result.applyClass))
			}
			joystick = on
			notice = nil
		} catch {
			notice = "The on-screen joystick could not be changed."
		}
	}
}
