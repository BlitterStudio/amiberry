/*
 * Copyright (C) 2026 BlitterStudio
 * Runtime actions and status (src/osdep/settings/amiberry_actions.h).
 * Actions can be posted from any thread; the emulator's main thread runs
 * them in order between frames. Status is a snapshot that can be read from
 * any thread.
 */

import AmiberrySettings
import Foundation
import Observation

/// An operation on the running emulator, as the built-in GUI and IPC do it.
public enum EmulatorAction: Hashable, Sendable {
	/// An empty path empties the drive.
	case insertFloppy(drive: Int, path: String)
	case ejectFloppy(drive: Int)
	case setSwapperEntry(index: Int, path: String)
	case insertSwapperEntry(index: Int, drive: Int)
	/// An empty path empties the drive.
	case insertCD(path: String)
	case ejectCD
	/// `hard` resets like a power cycle, `keyboard` like Ctrl-Amiga-Amiga.
	case reset(hard: Bool, keyboard: Bool)
	case pause
	case resume
	case saveState(path: String)
	case loadState(path: String)
	/// Quick-save slots 0-9.
	case saveStateSlot(Int)
	case loadStateSlot(Int)
	case screenshot(path: String)
	/// Makes a .uae file the pending configuration; it runs after
	/// `applySettings(.restart)`, or when the emulation next starts.
	case loadConfiguration(path: String)
	/// Writes the pending configuration (what a session committed).
	case saveConfiguration(path: String)
	/// Makes the machine for a media file (disk, CD, WHDLoad archive or
	/// configuration) the pending configuration; it runs as a loaded one does.
	case quickPlay(path: String)
	case quit
	/// Stops the running emulation and opens the Library with nothing
	/// running; fails while nothing runs.
	case stop
	/// Brings the running emulation up to the pending configuration with
	/// the apply class `SettingsSession.commit()` returned.
	case applySettings(ApplyClass)
	/// Switches the emulation display between window and full screen.
	case toggleFullscreen
	/// Opens the settings UI over a running emulation at its next frame;
	/// done at once while it is open or no emulation runs.
	case openSettings
	/// Asks the user for a file over a running emulation and uses it, as
	/// the media and state hotkeys do (the emulation pauses meanwhile).
	case chooseMedia(MediaRequest)
	/// Presses or releases a key of the emulated keyboard by Amiga raw key
	/// code (0x00-0x7F), as an on-screen keyboard does; every press needs its
	/// release. A press fails while nothing runs or the settings UI is open.
	case key(code: Int, pressed: Bool)
}

/// What `EmulatorAction.chooseMedia` asks the user for.
public enum MediaRequest: Hashable, Sendable {
	/// A disk image for DF0: to DF3:.
	case floppy(drive: Int)
	case loadState
	case saveState
	case cd

	var layerValue: amiberry_media_request? {
		switch self {
		case let .floppy(drive):
			guard drive >= 0, drive < Int(AMIBERRY_ACTIONS_FLOPPY_DRIVES) else { return nil }
			return amiberry_media_request(rawValue: AMIBERRY_REQUEST_FLOPPY0.rawValue + UInt32(drive))
		case .loadState: return AMIBERRY_REQUEST_LOAD_STATE
		case .saveState: return AMIBERRY_REQUEST_SAVE_STATE
		case .cd: return AMIBERRY_REQUEST_CD
		}
	}
}

/// What became of a posted action.
public enum ActionOutcome: Hashable, Sendable {
	/// Queued or executing.
	case pending
	case done
	/// Finished with an error; the text can be empty.
	case failed(String)
	/// Unknown, or its result was already taken by an earlier poll.
	case unknown
}

public enum ActionError: Error, Hashable, Sendable {
	/// Refused on the spot: an argument out of range, a path too long for
	/// the emulator, or a full queue. It never runs.
	case rejected(EmulatorAction)
	/// It ran and failed; the text can be empty.
	case failed(EmulatorAction, String)
	/// Its result was lost (retired before it was polled).
	case lost(EmulatorAction)
}

/// A posted action.
public struct ActionTicket: Hashable, Sendable {
	public let id: UInt64
	public let action: EmulatorAction

	/// Takes the action's result: exactly one poll sees `.done` or
	/// `.failed`; later polls see `.unknown`.
	public func poll() -> ActionOutcome {
		var error = [CChar](repeating: 0, count: 512)
		let result = error.withUnsafeMutableBufferPointer {
			amiberry_actions_poll(id, $0.baseAddress, $0.count)
		}
		switch result {
		case AMIBERRY_ACTION_PENDING: return .pending
		case AMIBERRY_ACTION_DONE: return .done
		case AMIBERRY_ACTION_FAILED: return .failed(decodedString(error))
		default: return .unknown
		}
	}
}

public enum EmulatorActions {
	/// The version of amiberry_actions.h this module was built against.
	public static let apiVersion = Int(AMIBERRY_ACTIONS_API_VERSION)

	/// Posts an action from any thread; nil when it was rejected on the spot.
	public static func post(_ action: EmulatorAction) -> ActionTicket? {
		let id: amiberry_action_id
		switch action {
		case let .insertFloppy(drive, path): id = amiberry_actions_insert_floppy(Int32(clamping: drive), path)
		case let .ejectFloppy(drive): id = amiberry_actions_eject_floppy(Int32(clamping: drive))
		case let .setSwapperEntry(index, path): id = amiberry_actions_set_swapper_entry(Int32(clamping: index), path)
		case let .insertSwapperEntry(index, drive):
			id = amiberry_actions_insert_swapper_entry(Int32(clamping: index), Int32(clamping: drive))
		case let .insertCD(path): id = amiberry_actions_insert_cd(path)
		case .ejectCD: id = amiberry_actions_eject_cd()
		case let .reset(hard, keyboard): id = amiberry_actions_reset(hard, keyboard)
		case .pause: id = amiberry_actions_pause()
		case .resume: id = amiberry_actions_resume()
		case let .saveState(path): id = amiberry_actions_save_state(path)
		case let .loadState(path): id = amiberry_actions_load_state(path)
		case let .saveStateSlot(slot): id = amiberry_actions_save_state_slot(Int32(clamping: slot))
		case let .loadStateSlot(slot): id = amiberry_actions_load_state_slot(Int32(clamping: slot))
		case let .screenshot(path): id = amiberry_actions_screenshot(path)
		case let .loadConfiguration(path): id = amiberry_actions_load_config(path)
		case let .saveConfiguration(path): id = amiberry_actions_save_config(path)
		case let .quickPlay(path): id = amiberry_actions_quick_play(path)
		case .quit: id = amiberry_actions_quit()
		case .stop: id = amiberry_actions_stop()
		case let .applySettings(apply): id = amiberry_actions_apply_settings(apply.layerValue)
		case .toggleFullscreen: id = amiberry_actions_toggle_fullscreen()
		case .openSettings: id = amiberry_actions_open_settings()
		case let .chooseMedia(request):
			guard let value = request.layerValue else { return nil }
			id = amiberry_actions_choose_media(value)
		case let .key(code, pressed):
			id = amiberry_actions_key(Int32(clamping: code), pressed)
		}
		return id == 0 ? nil : ActionTicket(id: id, action: action)
	}

	/// Posts an action and waits until the emulator ran it. While the
	/// native UI owns the main thread (the emulation is not running between
	/// frames), an `ActionPump` must be running for the action to execute.
	public static func perform(_ action: EmulatorAction, pollInterval: Duration = .milliseconds(5)) async throws {
		guard let ticket = post(action) else {
			throw ActionError.rejected(action)
		}
		while true {
			switch ticket.poll() {
			case .pending:
				try await Task.sleep(for: pollInterval)
			case .done:
				return
			case let .failed(message):
				throw ActionError.failed(action, message)
			case .unknown:
				throw ActionError.lost(action)
			}
		}
	}

	/// The pending disk-swapper entry; "" when unset. Any thread.
	public static func swapperEntry(_ index: Int) -> String {
		var buffer = [CChar](repeating: 0, count: Int(AMIBERRY_ACTIONS_PATH_MAX))
		buffer.withUnsafeMutableBufferPointer {
			amiberry_actions_get_swapper_entry(Int32(clamping: index), $0.baseAddress, $0.count)
		}
		return decodedString(buffer)
	}

	public static var swapperEntryCount: Int { Int(AMIBERRY_ACTIONS_SWAPPER_ENTRIES) }
	public static var floppyDriveCount: Int { Int(AMIBERRY_ACTIONS_FLOPPY_DRIVES) }
}

/// Runs the queued actions from the main run loop while the native UI owns
/// the main thread: in place of the built-in GUI, the emulator does not
/// drain the queue between frames.
@MainActor
public final class ActionPump {
	private var timer: Timer?

	public init() {}

	public var isRunning: Bool { timer != nil }

	public func start(interval: TimeInterval = 1.0 / 60.0) {
		guard timer == nil else { return }
		// A pump released without stop() ends its timer at the next tick.
		let timer = Timer(timeInterval: interval, repeats: true) { [weak self] timer in
			let alive = MainActor.assumeIsolated { () -> Bool in
				guard self != nil else { return false }
				_ = amiberry_actions_run_pending()
				return true
			}
			if !alive {
				timer.invalidate()
			}
		}
		RunLoop.main.add(timer, forMode: commonRunLoopModes)
		self.timer = timer
	}

	public func stop() {
		timer?.invalidate()
		timer = nil
	}

	/// Runs what is queued now; returns how many actions ran.
	@discardableResult
	public static func runPending() -> Int {
		amiberry_actions_run_pending()
	}
}

/// A snapshot of the running emulator.
public struct EmulatorStatus: Hashable, Sendable {
	public enum RunState: Hashable, Sendable {
		case stopped, running, paused
	}

	public struct Drive: Hashable, Sendable {
		/// The drive LED: the motor is spinning.
		public let motorOn: Bool
		public let writing: Bool
		public let disabled: Bool
		/// The pending configuration inserts media.
		public let mediaRequested: Bool
		/// The pending media path; "" when none.
		public let mediaPath: String
	}

	public let runState: RunState
	/// The raw pause flag.
	public let paused: Bool
	public let powerLED: Bool
	public let capsLock: Bool
	/// The hard drive LED level; nil when there is no hard drive.
	public let hardDriveActivity: Int?
	/// The CD LED level; nil when there is no CD drive.
	public let cdActivity: Int?
	/// Frames per second in tenths, as the status line shows them.
	public let fpsTenths: Int
	public let floppies: [Drive]
	public let cdRequested: Bool
	public let cdPath: String
	public let configDescription: String
	/// Increased by every refresh on the emulator's main thread, which is
	/// every frame while emulation runs.
	public let revision: UInt32

	/// Whether everything but the revision is the same.
	public func hasSameState(as other: EmulatorStatus) -> Bool {
		runState == other.runState && paused == other.paused && powerLED == other.powerLED
			&& capsLock == other.capsLock && hardDriveActivity == other.hardDriveActivity
			&& cdActivity == other.cdActivity && fpsTenths == other.fpsTenths && floppies == other.floppies && cdRequested == other.cdRequested
			&& cdPath == other.cdPath && configDescription == other.configDescription
	}

	/// The current snapshot; any thread.
	public static func current() -> EmulatorStatus {
		var raw = amiberry_actions_status()
		amiberry_actions_get_status(&raw)
		return EmulatorStatus(raw)
	}

	init(_ s: amiberry_actions_status) {
		switch s.run_state {
		case AMIBERRY_RUN_STATE_RUNNING: runState = .running
		case AMIBERRY_RUN_STATE_PAUSED: runState = .paused
		default: runState = .stopped
		}
		paused = s.paused
		powerLED = s.power_led
		capsLock = s.caps_lock
		hardDriveActivity = s.hd_activity < 0 ? nil : Int(s.hd_activity)
		cdActivity = s.cd_activity < 0 ? nil : Int(s.cd_activity)
		fpsTenths = Int(s.fps_tenths)
		floppies = withUnsafeBytes(of: s.floppy) { raw in
			raw.bindMemory(to: amiberry_drive_status.self).map {
				Drive(motorOn: $0.motor_on, writing: $0.writing, disabled: $0.disabled,
					mediaRequested: $0.media_requested, mediaPath: tupleString($0.media_path))
			}
		}
		cdRequested = s.cd_requested
		cdPath = tupleString(s.cd_path)
		configDescription = tupleString(s.config_description)
		revision = UInt32(s.revision)
	}
}

/// Publishes the emulator's status to SwiftUI, re-reading the snapshot on
/// the main run loop. `status` changes only when the state does, not with
/// every frame's revision.
@MainActor
@Observable
public final class StatusMonitor {
	public private(set) var status: EmulatorStatus

	@ObservationIgnored private var timer: Timer?

	public init() {
		status = EmulatorStatus.current()
	}

	public var isRunning: Bool { timer != nil }

	public func start(interval: TimeInterval = 0.1) {
		guard timer == nil else { return }
		let timer = Timer(timeInterval: interval, repeats: true) { [weak self] timer in
			let alive = MainActor.assumeIsolated { () -> Bool in
				guard let self else { return false }
				self.refresh()
				return true
			}
			if !alive {
				timer.invalidate()
			}
		}
		RunLoop.main.add(timer, forMode: commonRunLoopModes)
		self.timer = timer
	}

	public func stop() {
		timer?.invalidate()
		timer = nil
	}

	public func refresh() {
		let current = EmulatorStatus.current()
		if !current.hasSameState(as: status) {
			status = current
		}
	}
}

/// The common run loop modes (kCFRunLoopCommonModes), so polling timers also
/// fire during event tracking. Spelled out because `.common` does not
/// resolve against the iOS 18 SDK in Swift 6.1.
private let commonRunLoopModes = RunLoop.Mode("kCFRunLoopCommonModes")
