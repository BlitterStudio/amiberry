/*
 * Copyright (C) 2026 BlitterStudio
 * The WHDBoot tools API (src/osdep/settings/amiberry_whdboot.h): which of
 * the on-demand WHDBooter files are installed, and downloading them with
 * progress and cancellation. WHDLoad by Bert Jahn (whdload.de), the
 * kickstart RTB files, JST and the optional Amiga libraries (Aminet) are
 * not bundled with Amiberry, so the front ends offer the download the
 * Paths panel's "Update WHDBooter files" button performs.
 */

import AmiberrySettings
import Foundation

/// Which of the on-demand WHDBoot tools are installed.
public struct WHDBootStatus: Sendable {
	public let whdloadInstalled: Bool
	public let jstInstalled: Bool
	public let rtbFilesInstalled: Bool
	public let hostQuitInstalled: Bool
	public let bootDataInstalled: Bool
	public let databaseInstalled: Bool
	/// The optional Amiga libraries (XPK, 68040.library, explode.library,
	/// medplayer.library) downloaded from Aminet.
	public let librariesInstalled: Bool
	public let downloadsAvailable: Bool
	/// A machine runs unpaused: downloads must wait (the emulator refuses
	/// them; this lets a front end say why up front).
	public let emulationRunning: Bool
}

public enum WHDBootDownloadOutcome: Sendable {
	/// Everything was downloaded (WHDLoad is re-fetched on every update).
	case success
	/// Some files failed; WHDLoad itself may still have been installed.
	case partialFailure
	case failure
	case cancelled
	/// The build cannot download (libretro core).
	case unavailable
	/// Another download is running, or the machine is running.
	case busy
}

/// One download in progress; `cancel()` stops it. The download blocks the
/// thread that calls `run()` and reports progress from one of the
/// implementation's worker threads, so front ends start it off the main
/// actor and hop in `onProgress`.
public final class WHDBootDownload: @unchecked Sendable {
	private let lock = NSLock()
	private var cancelled = false
	/// (label, step, total steps, bytes done, bytes total; bytes total is 0
	/// while the size is unknown). Set before `run()`.
	public var onProgress: (@Sendable (String, Int, Int, Int64, Int64) -> Void)?

	public func cancel() {
		lock.lock()
		cancelled = true
		lock.unlock()
	}

	var isCancelled: Bool {
		lock.lock()
		defer { lock.unlock() }
		return cancelled
	}

	/// Blocks until the download finished.
	public func run() -> WHDBootDownloadOutcome {
		let box = Unmanaged.passRetained(self)
		let fn: amiberry_whdboot_progress_fn? = { progress, user in
			guard let user else { return 0 }
			let download = Unmanaged<WHDBootDownload>.fromOpaque(user).takeUnretainedValue()
			if let progress {
				download.onProgress?(String(cString: progress.pointee.label),
					Int(progress.pointee.step), Int(progress.pointee.total_steps),
					progress.pointee.bytes_downloaded, progress.pointee.bytes_total)
			}
			return download.isCancelled ? 1 : 0
		}
		let result = amiberry_whdboot_download(fn, box.toOpaque())
		box.release()
		switch result {
		case AMIBERRY_WHDBOOT_DOWNLOAD_OK: return .success
		case AMIBERRY_WHDBOOT_DOWNLOAD_PARTIAL: return .partialFailure
		case AMIBERRY_WHDBOOT_DOWNLOAD_CANCELLED: return .cancelled
		case AMIBERRY_WHDBOOT_DOWNLOAD_UNAVAILABLE: return .unavailable
		case AMIBERRY_WHDBOOT_DOWNLOAD_BUSY: return .busy
		default: return .failure
		}
	}
}

/// Runs one download at a time for the whole process (the emulator refuses
/// a second one anyway). App Settings and the Library share it, and a
/// download keeps running when the view that started it goes away.
@MainActor
public final class WHDBootDownloadController {
	public static let shared = WHDBootDownloadController()

	private var current: WHDBootDownload?

	public var isRunning: Bool { current != nil }

	/// Starts a download; nil when one already runs. The returned task
	/// finishes with the outcome.
	public func start(
		onProgress: (@Sendable (String, Int, Int, Int64, Int64) -> Void)?
	) -> Task<WHDBootDownloadOutcome, Never>? {
		guard current == nil else { return nil }
		let download = WHDBootDownload()
		download.onProgress = onProgress
		current = download
		// The controller is the process-wide one, so the task reaches it
		// through `shared` rather than capturing `self` across actors.
		return Task.detached {
			let outcome = download.run()
			await MainActor.run { WHDBootDownloadController.shared.current = nil }
			return outcome
		}
	}

	public func cancel() {
		current?.cancel()
	}
}

public enum WHDBootTools {
	/// The version of amiberry_whdboot.h this module was built against.
	public static let apiVersion = Int(AMIBERRY_WHDBOOT_API_VERSION)

	/// Which of the on-demand tools are installed in the WHDBoot folder.
	public static func status() -> WHDBootStatus {
		var raw = amiberry_whdboot_status()
		amiberry_whdboot_status_get(&raw)
		return WHDBootStatus(
			whdloadInstalled: raw.whdload_installed,
			jstInstalled: raw.jst_installed,
			rtbFilesInstalled: raw.rtb_files_installed,
			hostQuitInstalled: raw.host_quit_installed,
			bootDataInstalled: raw.boot_data_installed,
			databaseInstalled: raw.database_installed,
			librariesInstalled: raw.libraries_installed,
			downloadsAvailable: raw.downloads_available,
			emulationRunning: raw.emulation_running
		)
	}

	/// Formats a progress line the Library and the App Settings share.
	public static func progressText(label: String, step: Int, totalSteps: Int,
		bytesDone: Int64, bytesTotal: Int64) -> String {
		var text = "\(label) (\(max(step, 1))/\(max(totalSteps, 1)))"
		if bytesTotal > 0 {
			text += " — \(bytesDone / 1024) KB / \(bytesTotal / 1024) KB"
		}
		return text
	}
}
