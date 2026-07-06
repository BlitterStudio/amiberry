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

static void test_compressed_floppy_extensions_suggest_a500()
{
	const auto gz = play_detect_content("Disk.adf.gz", false);
	const auto xz = play_detect_content("disk.xz", false);

	expect_eq(gz.type, PlayContentType::Floppy, ".adf.gz must be detected as floppy");
	expect_eq(xz.type, PlayContentType::Floppy, ".xz must be detected as floppy");
	expect_eq(gz.suggested_model, PlaySuggestedModel::A500, "compressed floppies must suggest A500");
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

	expect_eq(cue.type, PlayContentType::Cd, ".cue must be detected as CD");
	expect_eq(cue.suggested_model, PlaySuggestedModel::Cd32, "CD content must suggest CD32");
	expect_eq(cue.follow_up, PlayFollowUp::ChooseCdMachine, "CD content must ask for CD machine choice");
}

static void test_chd_can_be_cd_or_hardfile()
{
	const auto detection = play_detect_content("Game.chd", false);

	expect_eq(detection.type, PlayContentType::Ambiguous, ".chd must allow CD or hardfile handling");
	expect_eq(detection.follow_up, PlayFollowUp::ChooseCdOrHardfile,
		".chd must ask whether it is CD media or a hardfile");
	expect_choice(detection.choices, PlayContentType::Cd, ".chd choices must include CD");
	expect_choice(detection.choices, PlayContentType::Hardfile, ".chd choices must include hardfile");
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

static void test_ambiguous_archive_floppy_action_suggests_a500()
{
	const auto detection = play_detect_content("game.zip", false);

	expect_eq(play_suggested_model_for_action(detection, PlayContentType::Floppy), PlaySuggestedModel::A500,
		"Ambiguous archives resolved as floppies must suggest A500");
}

static void test_aga_ambiguous_floppy_actions_suggest_a1200()
{
	const auto archive = play_detect_content("Game_AGA.zip", false);
	const auto disk = play_detect_content("Game_AGA.img", false);

	expect_eq(play_suggested_model_for_action(archive, PlayContentType::Floppy), PlaySuggestedModel::A1200,
		"AGA-named ambiguous archives resolved as floppies must suggest A1200");
	expect_eq(play_suggested_model_for_action(disk, PlayContentType::Floppy), PlaySuggestedModel::A1200,
		"AGA-named .img files resolved as floppies must suggest A1200");
}

static void test_direct_aga_floppy_action_keeps_a1200()
{
	const auto detection = play_detect_content("Game_AGA.adf", false);

	expect_eq(play_suggested_model_for_action(detection, PlayContentType::Floppy), PlaySuggestedModel::A1200,
		"Direct AGA floppies must keep the detected A1200 suggestion");
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
	test_compressed_floppy_extensions_suggest_a500();
	test_aga_floppy_names_suggest_a1200();
	test_cd_extensions_suggest_cd32_and_follow_up();
	test_chd_can_be_cd_or_hardfile();
	test_hardfile_extensions_attach_in_expert();
	test_unknown_extensions_remain_unknown();
	test_img_can_be_disk_or_hardfile();
	test_lha_archives_are_whdload();
	test_generic_archives_stay_ambiguous();
	test_ambiguous_archive_floppy_action_suggests_a500();
	test_aga_ambiguous_floppy_actions_suggest_a1200();
	test_direct_aga_floppy_action_keeps_a1200();
	test_directories_are_whdload_candidates();

	return failures == 0 ? 0 : 1;
}
