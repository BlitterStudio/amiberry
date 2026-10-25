/*
 * Copyright (C) 2026 BlitterStudio
 * The in-app licences screen: Amiberry's GPL-3 source offer, and every
 * third-party component the Apple build actually ships. The catalogue is
 * packaging/licences, copied into the bundle next to the privacy manifest.
 */

import SwiftUI

/// One shipped component, or one tool fetched on request and not part of Amiberry.
///
/// The bundled catalogue carries attribution only. The `note` field stays in
/// the model so the tests can prove none is shipped.
struct LicenceEntry: Identifiable, Decodable, Sendable {
	let id: String
	let name: String
	var version: String
	var copyright: String
	var licence: String
	var licenceName: String
	var url: String
	/// File name under texts/, empty when the entry has only a URL.
	var text: String
	var platforms: [String]
	var note: String

	private enum CodingKeys: String, CodingKey {
		case id, name, version, copyright, licence
		case licenceName = "licence_name"
		case url, text, platforms, note
	}

	init(from decoder: Decoder) throws {
		let values = try decoder.container(keyedBy: CodingKeys.self)
		id = try values.decode(String.self, forKey: .id)
		name = try values.decode(String.self, forKey: .name)
		version = try values.decodeIfPresent(String.self, forKey: .version) ?? ""
		copyright = try values.decodeIfPresent(String.self, forKey: .copyright) ?? ""
		licence = try values.decodeIfPresent(String.self, forKey: .licence) ?? ""
		licenceName = try values.decodeIfPresent(String.self, forKey: .licenceName) ?? licence
		url = try values.decodeIfPresent(String.self, forKey: .url) ?? ""
		text = try values.decodeIfPresent(String.self, forKey: .text) ?? ""
		platforms = try values.decodeIfPresent([String].self, forKey: .platforms) ?? []
		note = try values.decodeIfPresent(String.self, forKey: .note) ?? ""
	}
}

/// How this build identifies the exact source tree, written by the build.
struct SourceOffer: Decodable, Sendable {
	var version: String
	/// Empty when the revision could not be determined.
	var revision: String
	/// `tag`, `commit` or `unknown`.
	var kind: String
	/// True when the tree had local changes at build time.
	var dirty: Bool
	/// The tree URL, or the project URL when the revision is unknown.
	var url: String
	/// Release page carrying this build's third-party sources; empty when
	/// there is no tag to attach them to.
	var thirdPartyURL: String
	var projectURL: String
	var homepage: String
	/// `macos`, `macos-store` or `ios`: which catalogue rows this build ships.
	var distribution: String

	private enum CodingKeys: String, CodingKey {
		case version, revision, kind, dirty, url, homepage, distribution
		case thirdPartyURL = "third_party_url"
		case projectURL = "project_url"
	}
}

/// The bundled catalogue, its licence texts read once at load time.
struct LicencesDocument: Sendable {
	var source: SourceOffer
	var components: [LicenceEntry]
	var downloaded: [LicenceEntry]
	var directory: URL
	private var texts: [String: String]

	init(source: SourceOffer, components: [LicenceEntry], downloaded: [LicenceEntry],
		directory: URL, texts: [String: String]) {
		self.source = source
		self.components = components
		self.downloaded = downloaded
		self.directory = directory
		self.texts = texts
	}

	/// The licence text the entry names, or an empty string when it has none.
	func licenceText(for entry: LicenceEntry) -> String {
		guard !entry.text.isEmpty else { return "" }
		return texts[entry.id] ?? ""
	}

	func ships(_ entry: LicenceEntry) -> Bool {
		entry.platforms.isEmpty || entry.platforms.contains(source.distribution)
	}

	static func readText(_ name: String, in directory: URL) -> String {
		let url = directory.appendingPathComponent("texts").appendingPathComponent(name)
		return (try? String(contentsOf: url, encoding: .utf8)) ?? ""
	}
}

enum LicencesLoader {
	struct Failure: Error, LocalizedError, CustomStringConvertible {
		var description: String
		var errorDescription: String { description }
	}

	/// Reads the catalogue copied into the app bundle, with its texts.
	static func load(bundle: Bundle = .main) throws -> LicencesDocument {
		guard let directory = directory(in: bundle) else {
			throw Failure(description: "The licences catalogue is not in the app bundle.")
		}
		let catalogue = directory.appendingPathComponent("catalogue.json")
		let source = directory.appendingPathComponent("source-offer.json")
		let decoder = JSONDecoder()
		let document = try decoder.decode(CatalogueFile.self, from: Data(contentsOf: catalogue))
		let offer = try decoder.decode(SourceOffer.self, from: Data(contentsOf: source))
		guard !offer.url.isEmpty else {
			throw Failure(description: "The source offer has no URL.")
		}
		// Only the entries this distribution ships need their texts in the
		// bundle: the copy step filters texts per distribution.
		func ships(_ entry: LicenceEntry) -> Bool {
			entry.platforms.isEmpty || entry.platforms.contains(offer.distribution)
		}
		var texts: [String: String] = [:]
		for entry in document.components + document.downloaded where ships(entry) {
			var text = ""
			if !entry.text.isEmpty {
				text = LicencesDocument.readText(entry.text, in: directory)
				if text.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
					throw Failure(description: "\(entry.name) names a licence text that is missing (\(entry.text)).")
				}
			}
			if text.isEmpty && entry.url.isEmpty {
				throw Failure(description: "\(entry.name) has neither a licence text nor a URL.")
			}
			texts[entry.id] = text
		}
		return LicencesDocument(
			source: offer, components: document.components, downloaded: document.downloaded,
			directory: directory, texts: texts)
	}



	private struct CatalogueFile: Decodable {
		var components: [LicenceEntry]
		var downloaded: [LicenceEntry]
	}

	/// macOS keeps resources under Contents/Resources; the iOS bundle is flat.
	/// Both copy the catalogue into a `licences` directory.
	private static func directory(in bundle: Bundle) -> URL? {
		if let catalogue = bundle.url(forResource: "catalogue", withExtension: "json", subdirectory: "licences") {
			return catalogue.deletingLastPathComponent()
		}
		if let catalogue = bundle.url(forResource: "catalogue", withExtension: "json") {
			return catalogue.deletingLastPathComponent()
		}
		return nil
	}
}


/// Acknowledgements on the Mac, About & Licences on iOS: the GPL statement
/// and source offer, then the searchable component list with the licence
/// text of the selection.
struct LicencesView: View {
	@State private var document: LicencesDocument?
	@State private var failure: String?
	@State private var query = ""
	@State private var selection: String?

	var body: some View {
		Group {
			if let document {
				#if os(macOS)
				macBody(document)
				#else
				phoneBody(document)
				#endif
			} else if let failure {
				ContentUnavailableView(
					"Licences unavailable", systemImage: "exclamationmark.triangle", description: Text(failure))
			} else {
				ProgressView("Loading licences")
			}
		}
		.task { load() }
		.accessibilityIdentifier("licences.root")
	}

	private func load() {
		do {
			let loaded = try LicencesLoader.load()
			document = loaded
			if selection == nil {
				selection = loaded.components.first { $0.id == "amiberry" }?.id
			}
		} catch {
			failure = String(describing: error)
		}
	}

	private func matches(_ entry: LicenceEntry, query: String) -> Bool {
		let needle = query.trimmingCharacters(in: .whitespacesAndNewlines)
		guard !needle.isEmpty else { return true }
		let haystack = [entry.name, entry.copyright, entry.licence, entry.licenceName, entry.version].joined(separator: " ")
		return haystack.localizedStandardContains(needle)
	}

	// MARK: macOS

	#if os(macOS)
	private func macBody(_ document: LicencesDocument) -> some View {
		NavigationSplitView {
			List(selection: $selection) {
				// A plain field, not .searchable: a toolbar search item makes
				// the window title land in the sidebar's cramped title slot.
				Section {
					HStack(spacing: 6) {
						Image(systemName: "magnifyingglass").foregroundStyle(.secondary)
						TextField("Components", text: $query)
							.textFieldStyle(.plain)
					}
					.padding(6)
					.background(Color(nsColor: .controlBackgroundColor), in: RoundedRectangle(cornerRadius: 6))
				}
				Section("Shipped with this build") {
					ForEach(shipped(document)) { entry in
						row(entry).tag(entry.id)
					}
				}
				Section("Downloaded on request (not part of Amiberry)") {
					ForEach(downloaded(document)) { entry in
						row(entry).tag(entry.id)
					}
				}
			}
			.navigationSplitViewColumnWidth(min: 260, ideal: 300, max: 420)
			.accessibilityIdentifier("licences.list")
		} detail: {
			// The offer stays put above the licence scroll (a safeAreaInset
			// here starts the scroll offset inside the toolbar on macOS).
			VStack(spacing: 0) {
				SourceOfferHeader(document: document)
				Divider()
				if let entry = (document.components + document.downloaded).first(where: { $0.id == selection }) {
					LicenceDetail(entry: entry, text: document.licenceText(for: entry))
				} else {
					ContentUnavailableView("Select a component", systemImage: "doc.text")
				}
			}
			.navigationSplitViewColumnWidth(min: 560, ideal: 560)
			// The title lives in a principal toolbar item: the window title
			// itself would render in the sidebar segment, truncated.
			.toolbar {
				ToolbarItem(placement: .principal) {
					Text("Acknowledgements").font(.headline)
				}
				ToolbarItem(placement: .primaryAction) {
					Link(destination: URL(string: "https://github.com/BlitterStudio/amiberry")!) {
						Image(systemName: "arrow.up.right.square")
					}
					.help("Open the Amiberry repository")
				}
			}
		}
	}
	#endif

	// MARK: iOS

	#if os(iOS)
	private func phoneBody(_ document: LicencesDocument) -> some View {
		List {
			Section {
				SourceOfferHeader(document: document)
				NavigationLink {
					LicenceTextPage(title: "GNU General Public License", text: gplText(document))
				} label: {
					Label("Read the full GNU GPL", systemImage: "doc.plaintext")
				}
				.accessibilityIdentifier("licences.gpl")
			}
			Section("Shipped with this build") {
				ForEach(shipped(document)) { entry in
					NavigationLink {
						LicenceDetail(entry: entry, text: document.licenceText(for: entry))
					} label: {
						row(entry)
					}
				}
			}
			Section("Downloaded on request (not part of Amiberry)") {
				ForEach(downloaded(document)) { entry in
					NavigationLink {
						LicenceDetail(entry: entry, text: document.licenceText(for: entry))
					} label: {
						row(entry)
					}
				}
			}
		}
		.searchable(text: $query, prompt: "Components")
		.accessibilityIdentifier("licences.list")
	}
	#endif

	private func shipped(_ document: LicencesDocument) -> [LicenceEntry] {
		document.components.filter { document.ships($0) && matches($0, query: query) }
	}

	private func downloaded(_ document: LicencesDocument) -> [LicenceEntry] {
		document.downloaded.filter { matches($0, query: query) }
	}

	private func gplText(_ document: LicencesDocument) -> String {
		document.components.first { $0.id == "amiberry" }.map { document.licenceText(for: $0) } ?? ""
	}

	private func row(_ entry: LicenceEntry) -> some View {
		VStack(alignment: .leading, spacing: 2) {
			Text(entry.name)
			Text(entry.version.isEmpty ? entry.licenceName : "\(entry.version) · \(entry.licenceName)")
				.font(.caption)
				.foregroundStyle(.secondary)
		}
		.accessibilityElement(children: .combine)
		.accessibilityIdentifier("licences.row.\(entry.id)")
	}
}

/// The GPL statement (the notice the built-in About panel shows) and the
/// link to the exact source of this build. Each link is its own tap target.
struct SourceOfferHeader: View {
	let document: LicencesDocument

	private var projectURL: URL {
		URL(string: document.source.projectURL) ?? URL(string: "https://github.com/BlitterStudio/amiberry")!
	}
	private var homepageURL: URL {
		URL(string: document.source.homepage) ?? URL(string: "https://amiberry.com")!
	}
	private var sourceURL: URL {
		URL(string: document.source.url) ?? projectURL
	}

	var body: some View {
		VStack(alignment: .leading, spacing: 8) {
			Text("Copyright (C) 2016-2026 Dimitris Panokostas and the Amiberry contributors.")
			Text("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.")
			Text("This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.")
			revision(document.source)
			if !document.source.thirdPartyURL.isEmpty {
				Link("Source code of the third-party libraries in this build",
					destination: URL(string: document.source.thirdPartyURL) ?? projectURL)
					.buttonStyle(.borderless)
			} else {
				Text("The source code of the third-party libraries in this build is published with each release.")
					.font(.callout)
			}
			Link(document.source.revision.isEmpty ? "github.com/BlitterStudio/amiberry" : document.source.url,
				destination: sourceURL)
				.buttonStyle(.borderless)
			HStack(spacing: 16) {
				Link("Project", destination: projectURL).buttonStyle(.borderless)
				Link("amiberry.com", destination: homepageURL).buttonStyle(.borderless)
			}
			.font(.callout)
			#if os(macOS)
			Text("The full GNU GPL is shown when Amiberry is selected. The other rows are the third-party components this build ships, with the licence text each one requires.")
				.font(.caption)
				.foregroundStyle(.secondary)
			#endif
		}
		.padding(12)
		.frame(maxWidth: .infinity, alignment: .leading)
		.background(.bar)
		.textSelection(.enabled)
		.accessibilityIdentifier("licences.source")
	}

	/// The exact revision line: what it is, when it was built, and honesty
	/// when git could not name it.
	@ViewBuilder
	private func revision(_ offer: SourceOffer) -> some View {
		if offer.revision.isEmpty {
			Text("Version \(offer.version). The exact revision of this build could not be determined; the project repository is:")
				.font(.callout)
	} else {
		VStack(alignment: .leading, spacing: 2) {
			// The revision is one unbreakable token; kept on a line of its
			// own so it never truncates next to the words around it.
			Text("Version \(offer.version) — source for this build (\(offer.kind)):")
				.font(.callout)
			Text(offer.revision)
				.font(.system(.callout, design: .monospaced))
			if offer.dirty {
				Text("This build contains local changes on top of that revision.")
					.font(.caption)
					.foregroundStyle(.secondary)
			}
		}
	}
}
}

/// One component: who holds the copyright, the licence, and the text.
struct LicenceDetail: View {
	let entry: LicenceEntry
	let text: String

	var body: some View {
		ScrollView {
			VStack(alignment: .leading, spacing: 12) {
				Text(entry.name).font(.title2)
				if !entry.version.isEmpty {
					labeled("Version", entry.version)
				}
				if !entry.copyright.isEmpty {
					labeled("Copyright", entry.copyright)
				}
				labeled("Licence", entry.licenceName)
				if let url = URL(string: entry.url), !entry.url.isEmpty {
					Link(entry.url, destination: url).buttonStyle(.borderless)
				}
				if !text.isEmpty {
					Text("Licence text")
						.font(.headline)
					Text(text)
						.font(.system(.body, design: .monospaced))
						.textSelection(.enabled)
				}
			}
			.frame(maxWidth: .infinity, alignment: .leading)
			.padding(16)
		}
		.accessibilityIdentifier("licences.detail.\(entry.id)")
	}

	private func labeled(_ title: String, _ value: String) -> some View {
		VStack(alignment: .leading, spacing: 2) {
			Text(title).font(.caption).foregroundStyle(.secondary)
			Text(value).textSelection(.enabled)
		}
	}
}

#if os(iOS)
/// The full GPL (or any other long notice) on its own page.
struct LicenceTextPage: View {
	let title: String
	let text: String

	var body: some View {
		ScrollView {
			Text(text)
				.font(.system(.body, design: .monospaced))
				.textSelection(.enabled)
				.frame(maxWidth: .infinity, alignment: .leading)
				.padding(16)
		}
		.navigationTitle(title)
		.accessibilityIdentifier("licences.text")
	}
}
#endif
