#include <cstring>
#include <iostream>
#include <string>
#include <strings.h>

#include "expansion_display_name.h"

// Mirrors of the GUI site patterns that consume the helper (the sort
// comparator, combo preview and label builder in src/osdep/imgui/expansions.cpp,
// addhdcontroller() in src/osdep/amiberry_gui.cpp), kept local so this test
// compiles standalone like the other osdep unit tests.
static int failures;

// Typed null so the helper resolves like the production call sites, which
// always pass TCHAR fields, never bare nullptr.
static const char* const no_name = nullptr;

static void expect_streq(const char* actual, const char* expected, const char* message)
{
	if (std::strcmp(actual, expected) != 0) {
		std::cerr << message << ": expected \"" << expected << "\", got \"" << actual << "\"\n";
		failures++;
	}
}

// Label-builder mirror: the manufacturer suffix is only appended when it
// differs from the displayed name, exactly like the ImGui label builder and
// addhdcontroller() compare against the helper's result.
static std::string label_for(const char* friendlyname, const char* friendlymanufacturer)
{
	const char* display = expansion_display_name(friendlyname, friendlymanufacturer);
	std::string label = display;
	if (friendlymanufacturer && strcasecmp(friendlymanufacturer, display) != 0) {
		label += " (";
		label += friendlymanufacturer;
		label += ")";
	}
	return label;
}

int main()
{
	// Regression: expansion database entries may declare a NULL or empty
	// friendlyname. Every name that reaches the Expansions panel (sorting,
	// combo preview, labels) must go through expansion_display_name() so a
	// usable string is always rendered instead of crashing on the NULL
	// dereference (AE7).

	// A board with a friendly name renders it unchanged.
	expect_streq(expansion_display_name("Blizzard 1260", "Phase 5"), "Blizzard 1260",
		"Named board must render its friendlyname");
	expect_streq(expansion_display_name("Blizzard 1260", nullptr), "Blizzard 1260",
		"Named board without manufacturer must render its friendlyname");

	// NULL and empty friendlyname both fall back instead of reaching the GUI.
	expect_streq(expansion_display_name(no_name, no_name), "Unknown board",
		"Board without any name must render the fallback");
	expect_streq(expansion_display_name("", ""), "Unknown board",
		"Empty-string name must behave like NULL");
	expect_streq(expansion_display_name(nullptr, ""), "Unknown board",
		"Empty-string manufacturer must not be used as the name");
	expect_streq(expansion_display_name("", nullptr), "Unknown board",
		"Empty-string name with no manufacturer must behave like NULL");

	// Manufacturer-present-no-name boards render the manufacturer.
	expect_streq(expansion_display_name(nullptr, "Kupke"), "Kupke",
		"Unnamed board must fall back to its manufacturer");
	expect_streq(expansion_display_name("", "Kupke"), "Kupke",
		"Empty-string name must fall back to its manufacturer");

	// The crash-safety contract: whatever the database entry contains, the
	// helper never hands back NULL or an empty string, so downstream
	// consumers (_tcsicmp sorting, snprintf previews, label builders) can
	// never dereference NULL.
	const char* names[] = { nullptr, "", "Blizzard 1260" };
	const char* manufacturers[] = { nullptr, "", "Phase 5" };
	for (const char* name : names) {
		for (const char* manufacturer : manufacturers) {
			const char* display = expansion_display_name(name, manufacturer);
			if (display == nullptr || display[0] == '\0') {
				std::cerr << "Helper result must never be NULL or empty\n";
				failures++;
			}
		}
	}

	// Sort-comparator mirror (RefreshExpansionList): both operands come from
	// the helper, so NULL-friendlyname boards no longer feed NULL to
	// _tcsicmp; two unnamed boards compare equal and the fallback sorts
	// deterministically against named boards.
	const char* unnamed = expansion_display_name(no_name, no_name);
	const char* named = expansion_display_name("Blizzard 1260", nullptr);
	if (std::strcmp(unnamed, unnamed) != 0 || std::strcmp(unnamed, named) == 0) {
		std::cerr << "Sorting must compare helper output, never raw friendlyname\n";
		failures++;
	}

	// Label-builder mirror: unnamed boards render a safe label, a board
	// already rendered under its manufacturer does not grow a duplicate
	// "(Manufacturer)" suffix, and the suffix is preserved where it belongs.
	expect_streq(label_for(nullptr, nullptr).c_str(), "Unknown board",
		"Unnamed board label must render the fallback");
	expect_streq(label_for(nullptr, "Kupke").c_str(), "Kupke",
		"Manufacturer fallback label must not duplicate the manufacturer");
	expect_streq(label_for("", "Kupke").c_str(), "Kupke",
		"Empty-name manufacturer label must not duplicate the manufacturer");
	expect_streq(label_for("Blizzard 1260", nullptr).c_str(), "Blizzard 1260",
		"Named board without manufacturer keeps its plain label");
	expect_streq(label_for("Blizzard 1260", "Phase 5").c_str(), "Blizzard 1260 (Phase 5)",
		"Named board with a different manufacturer keeps the suffix");
	return failures == 0 ? 0 : 1;
}

