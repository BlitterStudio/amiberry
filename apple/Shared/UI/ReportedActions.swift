/*
 * Copyright (C) 2026 BlitterStudio
 * Runtime actions posted from the running emulation's commands (the Mac's
 * menus and toolbar, the iOS running sheet), with their failures shown.
 */

@MainActor
enum ReportedActions {
	/// Posts an action; when it is refused, or fails when it runs (an empty
	/// quick-save slot, an unwritable screenshot folder), says so in a sheet,
	/// so the emulation keeps running while it shows.
	static func post(_ action: EmulatorAction) {
		guard let ticket = EmulatorActions.post(action) else {
			report("The emulator did not accept the request. Try again.")
			return
		}
		Task { @MainActor in
			while true {
				switch ticket.poll() {
				case .pending:
					try? await Task.sleep(for: .milliseconds(50))
				case let .failed(message):
					report(message.isEmpty ? "The command failed." : message)
					return
				case .done, .unknown:
					return
				}
			}
		}
	}

	private static func report(_ message: String) {
		Alerts.showSheet(title: "Command Failed", message: message)
	}
}
