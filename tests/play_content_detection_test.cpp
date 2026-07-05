#include <iostream>
#include <vector>

#include "imgui/play_content_detection.h"

static int failures;

template<typename T>
static void expect_eq(T actual, T expected, const char *message)
{
	if (actual != expected) {
		std::cerr << message << '\n';
		failures++;
	}
}

static void expect_choice(const std::vector<PlayContentType>& choices, PlayContentType expected,
	const char *message)
{
	for (const auto choice : choices) {
		if (choice == expected) {
			return;
		}
	}
	std::cerr << message << '\n';
	failures++;
}

static void test_configurations_keep_existing_model()
{
	const auto detection = play_detect_content("Workbench.uae", false);

	expect_eq(detection.type, PlayContentType::Configuration, ".uae must be detected as configuration");
	expect_eq(detection.suggested_model, PlaySuggestedModel::KeepExisting,
		".uae must keep the existing model");
}

static void test_floppy_extensions_suggest_a500()
{
	const auto adf = play_detect_content("Disk.ADF", false);
	const auto ipf = play_detect_content("disk.ipf", false);

	expect_eq(adf.type, PlayContentType::Floppy, ".ADF must be detected as floppy");
	expect_eq(ipf.type, PlayContentType::Floppy, ".ipf must be detected as floppy");
	expect_eq(adf.suggested_model, PlaySuggestedModel::A500, "floppies must suggest A500");
}

static void test_aga_floppy_names_suggest_a1200()
{
	const auto detection = play_detect_content("Game_AGA.adf", false);

	expect_eq(detection.type, PlayContentType::Floppy, "AGA-named .adf must still be detected as floppy");
	expect_eq(detection.suggested_model, PlaySuggestedModel::A1200,
		"AGA-named floppy content must suggest A1200");
}

static void test_cd_extensions_suggest_cd32_and_follow_up()
{
	const auto cue = play_detect_content("Game.cue", false);
	const auto chd = play_detect_content("Game.chd", false);

	expect_eq(cue.type, PlayContentType::Cd, ".cue must be detected as CD");
	expect_eq(chd.type, PlayContentType::Cd, ".chd must be detected as CD");
	expect_eq(cue.suggested_model, PlaySuggestedModel::Cd32, "CD content must suggest CD32");
	expect_eq(cue.follow_up, PlayFollowUp::ChooseCdMachine, "CD content must ask for CD machine choice");
}

static void test_hardfile_extensions_attach_in_expert()
{
	const auto detection = play_detect_content("Workbench.hdf", false);

	expect_eq(detection.type, PlayContentType::Hardfile, ".hdf must be detected as hardfile");
	expect_eq(detection.suggested_model, PlaySuggestedModel::A1200Expanded,
		"hardfiles must suggest A1200 expanded");
	expect_eq(detection.follow_up, PlayFollowUp::AttachHardfileInExpert,
		"hardfiles must be attached through expert follow-up");
}

static void test_unknown_extensions_remain_unknown()
{
	const auto detection = play_detect_content("notes.txt", false);

	expect_eq(detection.type, PlayContentType::Unknown, "unknown extensions must stay unknown");
}

static void test_img_can_be_disk_or_hardfile()
{
	const auto detection = play_detect_content("disk.img", false);

	expect_eq(detection.type, PlayContentType::Ambiguous, ".img must be ambiguous");
	expect_eq(detection.follow_up, PlayFollowUp::ChooseDiskOrHardfile,
		".img must ask whether it is a disk or hardfile");
	expect_choice(detection.choices, PlayContentType::Floppy, ".img choices must include floppy");
	expect_choice(detection.choices, PlayContentType::Hardfile, ".img choices must include hardfile");
}

static void test_lha_archives_are_whdload()
{
	const auto detection = play_detect_content("game.lha", false);

	expect_eq(detection.type, PlayContentType::WhdLoad, ".lha must be detected as WHDLoad");
	expect_eq(detection.suggested_model, PlaySuggestedModel::A1200, ".lha must suggest A1200");
	expect_eq(detection.follow_up, PlayFollowUp::None,
		".lha must not ask for archive content type");
}

static void test_generic_archives_stay_ambiguous()
{
	const auto detection = play_detect_content("game.zip", false);

	expect_eq(detection.type, PlayContentType::Ambiguous, ".zip can still contain disk images or WHDLoad content");
	expect_eq(detection.follow_up, PlayFollowUp::ChooseArchiveContent,
		".zip must ask for archive content type");
	expect_choice(detection.choices, PlayContentType::WhdLoad, ".zip choices must include WHDLoad");
	expect_choice(detection.choices, PlayContentType::Floppy, ".zip choices must include floppy");
}

static void test_directories_are_whdload_candidates()
{
	const auto detection = play_detect_content("Games/AlienBreed", true);

	expect_eq(detection.type, PlayContentType::WhdLoad, "directories must be WHDLoad candidates");
	expect_eq(detection.suggested_model, PlaySuggestedModel::A1200, "directories must suggest A1200");
	expect_eq(detection.suggested_compatibility, 1, "directories must keep compatibility level 1");
}

int main()
{
	test_configurations_keep_existing_model();
	test_floppy_extensions_suggest_a500();
	test_aga_floppy_names_suggest_a1200();
	test_cd_extensions_suggest_cd32_and_follow_up();
	test_hardfile_extensions_attach_in_expert();
	test_unknown_extensions_remain_unknown();
	test_img_can_be_disk_or_hardfile();
	test_lha_archives_are_whdload();
	test_generic_archives_stay_ambiguous();
	test_directories_are_whdload_candidates();

	return failures == 0 ? 0 : 1;
}
