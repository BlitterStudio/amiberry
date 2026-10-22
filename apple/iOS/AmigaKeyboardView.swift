/*
 * Copyright (C) 2026 BlitterStudio
 * The on-screen Amiga keyboard (layout and key logic: apple/Shared/UI/
 * AmigaKeyboard.swift): the main block with the cursor keys, and the
 * numeric keypad when the amiberry.vkbd_numpad option asks for it. Several
 * keys can be held at once.
 */

import SwiftUI

/// The on-screen keyboard options of the running configuration.
struct KeyboardOptions: Equatable {
	var enabled = false
	var keypad = false
	var layout = "US"
	/// 0...1.
	var opacity = 0.85

	/// The values the emulation uses now.
	@MainActor
	static func active() -> KeyboardOptions {
		func value(_ id: String) -> String? { try? SettingsSession.activeValue(of: id) }
		var options = KeyboardOptions()
		options.enabled = value("amiberry.vkbd_enabled") == "true"
		options.keypad = value("amiberry.vkbd_numpad") == "true"
		options.layout = value("amiberry.vkbd_language") ?? "US"
		// 0 means the default.
		if let percent = value("amiberry.vkbd_transparency").flatMap(Double.init), percent > 0 {
			options.opacity = min(percent, 100) / 100
		}
		return options
	}
}

struct AmigaKeyboardView: View {
	let model: AmigaKeyboardModel
	let options: KeyboardOptions

	private static let gap: CGFloat = 4
	private static let separation: CGFloat = 10

	private var blocks: [[[AmigaKey?]]] {
		var blocks: [[[AmigaKey?]]] = [AmigaKeyboardLayout.main.map { $0.map { $0 } }, AmigaKeyboardLayout.cursor]
		if options.keypad {
			blocks.append(AmigaKeyboardLayout.keypad.map { $0.map { $0 } })
		}
		return blocks
	}

	private static func units(_ block: [[AmigaKey?]]) -> Double {
		block.map { $0.reduce(0) { $0 + ($1?.width ?? 1) } }.max() ?? 0
	}

	var body: some View {
		GeometryReader { geometry in
			let blocks = self.blocks
			let rows = CGFloat(AmigaKeyboardLayout.main.count)
			let width = blocks.reduce(0) { $0 + Self.units($1) }
			let unit = min(
				(geometry.size.width - Self.separation * CGFloat(blocks.count - 1) - Self.gap * 2) / width,
				(geometry.size.height - Self.gap * (rows + 1)) / rows)
			HStack(alignment: .top, spacing: Self.separation) {
				ForEach(blocks.indices, id: \.self) { index in
					block(blocks[index], unit: unit)
				}
			}
			.padding(Self.gap)
			.frame(width: geometry.size.width, height: geometry.size.height)
		}
		// At most half the screen, so the emulation stays in view.
		.frame(maxHeight: min(280, UIScreen.main.bounds.height / 2))
		.background(.ultraThinMaterial)
		.opacity(options.opacity)
	}

	private func block(_ rows: [[AmigaKey?]], unit: CGFloat) -> some View {
		VStack(alignment: .leading, spacing: Self.gap) {
			ForEach(rows.indices, id: \.self) { row in
				HStack(spacing: Self.gap) {
					ForEach(rows[row].indices, id: \.self) { column in
						if let key = rows[row][column] {
							KeyView(key: key, layout: options.layout, model: model)
								.frame(width: unit * key.width - Self.gap, height: unit - Self.gap)
						} else {
							Color.clear.frame(width: unit - Self.gap, height: unit - Self.gap)
						}
					}
				}
				.frame(height: unit - Self.gap)
			}
		}
	}
}

private struct KeyView: View {
	let key: AmigaKey
	let layout: String
	let model: AmigaKeyboardModel
	/// Resets when the touch ends or is cancelled (a system gesture, too
	/// many fingers), which releases the key; onEnded alone misses cancels.
	@GestureState private var touching = false

	var body: some View {
		let down = model.down.contains(key.code)
		let legend = AmigaKeyboardLayout.legend(of: key, layout: layout)
		ZStack {
			RoundedRectangle(cornerRadius: 5)
				.fill(down ? Color.accentColor : Color(white: 0.92))
			VStack(spacing: 0) {
				if let shifted = legend.shifted {
					Text(shifted).font(.system(size: 10))
				}
				Text(legend.label).font(.system(size: legend.label.count > 2 ? 11 : 15, weight: .medium))
					.minimumScaleFactor(0.5)
			}
			.foregroundStyle(down ? Color.white : Color.black)
			.lineLimit(1)
		}
		.contentShape(Rectangle())
		.gesture(DragGesture(minimumDistance: 0).updating($touching) { _, touching, _ in touching = true })
		.onChange(of: touching) {
			if touching {
				model.touchDown(key)
			} else {
				model.touchUp(key)
			}
		}
		.accessibilityLabel(legend.label.isEmpty ? "Space" : legend.label)
		.accessibilityAddTraits(.isButton)
	}
}
