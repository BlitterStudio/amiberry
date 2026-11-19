#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "amiberry_whdload_icon.h"

static int failures;

static void expect(const bool condition, const char* message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

static void put16(std::vector<uint8_t>& v, const size_t offset, const uint32_t value)
{
	v[offset] = static_cast<uint8_t>(value >> 8);
	v[offset + 1] = static_cast<uint8_t>(value);
}

static void put32(std::vector<uint8_t>& v, const size_t offset, const uint32_t value)
{
	put16(v, offset, value >> 16);
	put16(v, offset + 2, value & 0xffff);
}

static void append32(std::vector<uint8_t>& v, const uint32_t value)
{
	v.resize(v.size() + 4);
	put32(v, v.size() - 4, value);
}

static void append_string(std::vector<uint8_t>& v, const std::string& s)
{
	append32(v, static_cast<uint32_t>(s.size() + 1));
	v.insert(v.end(), s.begin(), s.end());
	v.push_back(0);
}

static void append_image(std::vector<uint8_t>& v, const uint16_t width, const uint16_t height, const uint16_t depth)
{
	const size_t at = v.size();
	v.resize(at + 20);
	put16(v, at + 4, width);
	put16(v, at + 6, height);
	put16(v, at + 8, depth);
	put32(v, at + 10, 1); // ImageData
	v.resize(v.size() + (width + 15) / 16 * 2 * height * depth, 0x55);
}

// A Workbench project icon laid out as icon.library writes it.
static std::vector<uint8_t> make_icon(const bool drawer_data, const bool select_image,
	const std::string& default_tool, const std::vector<std::string>& tooltypes)
{
	std::vector<uint8_t> v(78);
	put16(v, 0, 0xe310);
	put16(v, 2, 1);
	put32(v, 22, 1); // GadgetRender
	put32(v, 26, select_image ? 1 : 0);
	v[48] = 4; // WBPROJECT
	put32(v, 50, default_tool.empty() ? 0 : 1);
	put32(v, 54, tooltypes.empty() ? 0 : 1);
	put32(v, 66, drawer_data ? 1 : 0);
	if (drawer_data)
		v.resize(v.size() + 56);
	append_image(v, 17, 3, 2);
	if (select_image)
		append_image(v, 8, 2, 1);
	if (!default_tool.empty())
		append_string(v, default_tool);
	if (!tooltypes.empty()) {
		append32(v, static_cast<uint32_t>((tooltypes.size() + 1) * 4));
		for (const auto& t : tooltypes)
			append_string(v, t);
	}
	return v;
}

static WhdloadIcon icon(const std::string& name, const std::vector<std::string>& tooltypes)
{
	return { name, whdload_icon_args(tooltypes) };
}

static void test_tooltypes()
{
	const std::vector<std::string> expected = {
		"SLAVE=BattleIsle.Slave", "CUSTOM1=3", "PRELOAD", "(CUSTOM2=7)", "custom3=-1",
		"CUSTOM=\"a b\"", "CUSTOM4=x", "IM1=abc"
	};
	for (const bool drawer : { false, true }) {
		for (const bool select : { false, true }) {
			const auto data = make_icon(drawer, select, "WHDLoad", expected);
			std::vector<std::string> tooltypes;
			expect(whdload_icon_tooltypes(data.data(), data.size(), tooltypes), "a valid icon must parse");
			expect(tooltypes == expected, "tool types must come back in order");
		}
	}

	std::vector<std::string> tooltypes;
	const auto data = make_icon(false, true, "WHDLoad", expected);
	expect(!whdload_icon_tooltypes(data.data(), data.size() - 3, tooltypes), "a cut icon must fail");

	auto not_icon = data;
	not_icon[0] = 0;
	expect(!whdload_icon_tooltypes(not_icon.data(), not_icon.size(), tooltypes), "a non-icon must fail");

	const auto no_tooltypes = make_icon(false, false, "MultiView", {});
	expect(whdload_icon_tooltypes(no_tooltypes.data(), no_tooltypes.size(), tooltypes) && tooltypes.empty(),
		"an icon without tool types parses to none");

	const auto args = whdload_icon_args(expected);
	expect(args.slave == "BattleIsle.Slave", "SLAVE");
	expect(args.custom[0] == 3, "CUSTOM1");
	expect(!args.custom[1], "a disabled tool type must be skipped");
	expect(args.custom[2] == -1, "keys are case-insensitive and values signed");
	expect(!args.custom[3], "a non-number CUSTOM4 must be skipped");
	expect(args.custom_text == std::string("a b"), "CUSTOM text loses its quotes");
}

static void test_pick()
{
	// Retroplay's BattleIsle: the game, its intro and a text viewer icon.
	const std::vector<WhdloadIcon> battle_isle = {
		icon("Intro", { "SLAVE=BattleIsle.Slave", "CUSTOM1=1" }),
		icon("BattleIsle", { "SLAVE=BattleIsle.Slave", "CUSTOM1=3" }),
		icon("ReadMe", {}),
	};
	const auto* picked = whdload_pick_icon(battle_isle, "BattleIsle", "BattleIsle.Slave");
	expect(picked && picked->name == "BattleIsle", "the icon named after the drawer wins");

	// The data disk pack: no icon is named after the drawer.
	const std::vector<WhdloadIcon> data_disks = {
		icon("AirLandSea", { "SLAVE=BattleIsle&DataDisks.slave", "CUSTOM1=4" }),
		icon("BattleIsle", { "SLAVE=BattleIsle&DataDisks.slave", "CUSTOM1=3" }),
		icon("Intro", { "SLAVE=BattleIsle&DataDisks.slave", "CUSTOM1=1" }),
		icon("MoonOfChromos", { "SLAVE=BattleIsle&DataDisks.slave", "CUSTOM1=5" }),
	};
	picked = whdload_pick_icon(data_disks, "BattleIsle&DataDisks", "BattleIsle&DataDisks.Slave");
	expect(picked && picked->name == "BattleIsle", "the icon naming the drawer's leading part wins");

	const std::vector<WhdloadIcon> sequel = {
		icon("Lemmings", { "SLAVE=Lemmings2.slave", "CUSTOM1=1" }),
		icon("Tribes", { "SLAVE=Lemmings2.slave", "CUSTOM1=2" }),
	};
	expect(!whdload_pick_icon(sequel, "Lemmings2", "Lemmings2.slave"),
		"a leading part must end at a separator, and different icons are ambiguous");

	const std::vector<WhdloadIcon> same = {
		icon("Start", { "SLAVE=Game.slave", "CUSTOM1=2" }),
		icon("Play", { "SLAVE=Game.slave", "CUSTOM1=2" }),
	};
	expect(whdload_pick_icon(same, "Drawer", "Game.slave") != nullptr, "icons that agree are not ambiguous");

	// Without a SLAVE tool type WHDLoad starts <icon name>.slave.
	const std::vector<WhdloadIcon> implicit = { icon("Game", { "CUSTOM2=1" }) };
	picked = whdload_pick_icon(implicit, "Other", "game.Slave");
	expect(picked && picked->args.custom[1] == 1, "the icon name names the slave by default");
	expect(!whdload_pick_icon(implicit, "Other", "Another.slave"), "an icon for another slave is ignored");
}

int main()
{
	test_tooltypes();
	test_pick();
	if (failures) {
		std::cerr << failures << " WHDLoad icon check(s) failed\n";
		return 1;
	}
	std::cout << "WHDLoad icon tests passed\n";
	return 0;
}
