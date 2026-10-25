/*
 * Copyright (C) 2026 BlitterStudio
 * The bundled licences catalogue must load, name the build's distribution,
 * ship the Amiberry (GPL) entry, and carry no compliance notes (attribution
 * only).
 */

@testable import AmiberryNativeUI
import XCTest

final class LicencesCatalogueTests: XCTestCase {
	func testBundledCatalogueLoads() throws {
		let document = try LicencesLoader.load()
		XCTAssertEqual(document.source.distribution, "macos")
		let amiberry = try XCTUnwrap(document.components.first { $0.id == "amiberry" })
		XCTAssertTrue(document.ships(amiberry))
	}

	func testNoShippedEntryCarriesAuditNotes() throws {
		let document = try LicencesLoader.load()
		for entry in document.components where document.ships(entry) {
			XCTAssertTrue(entry.note.isEmpty, "\(entry.id) ships a note: \(entry.note)")
		}
	}
}
