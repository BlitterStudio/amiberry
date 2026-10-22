/*
 * Copyright (C) 2026 BlitterStudio
 * String plumbing between Swift and the C APIs of the settings layer.
 */

import AmiberrySettings

/// A C string the layer owns for the life of the process, or "" for NULL.
func layerString(_ pointer: UnsafePointer<CChar>?) -> String {
	guard let pointer else { return "" }
	return String(cString: pointer)
}

/// A C string the layer owns, or nil for NULL.
func optionalLayerString(_ pointer: UnsafePointer<CChar>?) -> String? {
	guard let pointer else { return nil }
	return String(cString: pointer)
}

/// The largest buffer `readLayerString` offers: well above any path the
/// emulator can store (AMIBERRY_ACTIONS_PATH_MAX).
let layerStringLimit = 1 << 16

/// Calls `fill` with a buffer and its size until the layer stops reporting
/// AMIBERRY_SETTINGS_BUFFER_TOO_SMALL, doubling the buffer from `capacity`,
/// and returns the text with the final status. A value that does not fit
/// in `layerStringLimit` bytes is reported as BUFFER_TOO_SMALL.
func readLayerString(
	capacity: Int = Int(AMIBERRY_SETTINGS_VALUE_MAX),
	_ fill: (UnsafeMutablePointer<CChar>, Int) -> amiberry_settings_status
) -> (String, amiberry_settings_status) {
	var size = max(capacity, 1)
	while true {
		var buffer = [CChar](repeating: 0, count: size)
		let status = buffer.withUnsafeMutableBufferPointer { fill($0.baseAddress!, $0.count) }
		if status != AMIBERRY_SETTINGS_BUFFER_TOO_SMALL {
			let text = status == AMIBERRY_SETTINGS_OK ? decodedString(buffer) : ""
			return (text, status)
		}
		if size >= layerStringLimit {
			return ("", status)
		}
		size = min(size * 2, layerStringLimit)
	}
}

/// The NUL-terminated UTF-8 text at the start of a C character buffer;
/// invalid UTF-8 is repaired rather than dropped.
func decodedString(_ buffer: [CChar]) -> String {
	buffer.withUnsafeBufferPointer { pointer in
		let length = pointer.firstIndex(of: 0) ?? pointer.count
		let bytes = UnsafeRawBufferPointer(rebasing: UnsafeRawBufferPointer(pointer)[0..<length])
		return String(decoding: bytes, as: UTF8.self)
	}
}

/// The text of a fixed-size C character array imported as a tuple, such
/// as the adjustment and status fields.
func tupleString<T>(_ tuple: T) -> String {
	withUnsafeBytes(of: tuple) { raw in
		let length = raw.firstIndex(of: 0) ?? raw.count
		return String(decoding: raw[0..<length], as: UTF8.self)
	}
}
