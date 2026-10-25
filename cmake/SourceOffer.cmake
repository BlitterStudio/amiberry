# Generates amiberry-source-offer.json at BUILD time (run by an
# always-executed custom target), so the offer always names the exact
# revision the binary was built from - even when HEAD moved without a
# CMake reconfigure, and even when the executable does not relink.
#
# The file is rewritten only when its contents change, so an unchanged
# tree does not dirty dependent steps every build.
#
# Tag rule: an exact `v[0-9]*` tag on a clean, non-pre-release HEAD names
# the release; anything else names the commit (marked -dirty when the tree
# has local changes). Without git, the revision is honestly unknown.

set(_saved_git_dir "$ENV{GIT_DIR}")
unset(ENV{GIT_DIR})

find_program(_amiberry_git NAMES git)

set(AMIBERRY_SOURCE_KIND "unknown")
set(AMIBERRY_SOURCE_REVISION "")
set(_amiberry_dirty "false")

if(_amiberry_git)
	execute_process(
		COMMAND "${_amiberry_git}" -C "${SRC_DIR}" rev-parse HEAD
		RESULT_VARIABLE _amiberry_rev_result
		OUTPUT_VARIABLE _amiberry_rev
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_VARIABLE _amiberry_rev_error)
	if(_amiberry_rev_result EQUAL 0 AND NOT _amiberry_rev STREQUAL "")
		set(AMIBERRY_SOURCE_KIND "commit")
		set(AMIBERRY_SOURCE_REVISION "${_amiberry_rev}")
	else()
		message(STATUS "Source offer: git rev-parse failed: ${_amiberry_rev_error}")
	endif()

	if(NOT PRERELEASE)
		execute_process(
			COMMAND "${_amiberry_git}" -C "${SRC_DIR}" describe --exact-match --tags --match "v[0-9]*"
			RESULT_VARIABLE _amiberry_tag_result
			OUTPUT_VARIABLE _amiberry_tag
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET)
		if(_amiberry_tag_result EQUAL 0 AND NOT _amiberry_tag STREQUAL "")
			set(AMIBERRY_SOURCE_KIND "tag")
			set(AMIBERRY_SOURCE_REVISION "${_amiberry_tag}")
		endif()
	endif()

	# --ignore-submodules: a worktree whose submodule checkout is missing
	# must not make the dirty check fail.
	execute_process(
		COMMAND "${_amiberry_git}" -C "${SRC_DIR}" status --porcelain --ignore-submodules=all
		RESULT_VARIABLE _amiberry_status_result
		OUTPUT_VARIABLE _amiberry_status
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET)
	if(_amiberry_status_result EQUAL 0 AND NOT _amiberry_status STREQUAL "")
		set(_amiberry_dirty "true")
		if(NOT AMIBERRY_SOURCE_REVISION STREQUAL "")
			string(APPEND AMIBERRY_SOURCE_REVISION "-dirty")
		endif()
	elseif(NOT _amiberry_status_result EQUAL 0)
		message(STATUS "Source offer: git status failed; dirtiness unknown")
	endif()
endif()

if(_saved_git_dir)
	set(ENV{GIT_DIR} "${_saved_git_dir}")
endif()

if(AMIBERRY_SOURCE_REVISION STREQUAL "")
	set(AMIBERRY_SOURCE_URL "https://github.com/BlitterStudio/amiberry")
else()
	set(AMIBERRY_SOURCE_URL "https://github.com/BlitterStudio/amiberry/tree/${AMIBERRY_SOURCE_REVISION}")
endif()

if(AMIBERRY_SOURCE_KIND STREQUAL "tag")
	set(AMIBERRY_THIRD_PARTY_URL "https://github.com/BlitterStudio/amiberry/releases/tag/${AMIBERRY_SOURCE_REVISION}")
else()
	set(AMIBERRY_THIRD_PARTY_URL "")
endif()

# The substituted values are project-controlled, but keep quotes out anyway.
foreach(_amiberry_value AMIBERRY_PRODUCT_VERSION AMIBERRY_SOURCE_REVISION AMIBERRY_SOURCE_KIND AMIBERRY_DISTRIBUTION)
	string(REPLACE "\"" "" ${_amiberry_value} "${${_amiberry_value}}")
endforeach()

set(_amiberry_content
"{
  \"version\": \"${AMIBERRY_PRODUCT_VERSION}\",
  \"revision\": \"${AMIBERRY_SOURCE_REVISION}\",
  \"kind\": \"${AMIBERRY_SOURCE_KIND}\",
  \"dirty\": ${_amiberry_dirty},
  \"url\": \"${AMIBERRY_SOURCE_URL}\",
  \"third_party_url\": \"${AMIBERRY_THIRD_PARTY_URL}\",
  \"project_url\": \"https://github.com/BlitterStudio/amiberry\",
  \"homepage\": \"https://amiberry.com\",
  \"distribution\": \"${AMIBERRY_DISTRIBUTION}\"
}
")

if(EXISTS "${OUT_FILE}")
	file(READ "${OUT_FILE}" _amiberry_existing)
	if(_amiberry_existing STREQUAL _amiberry_content)
		message(STATUS "Source offer unchanged: ${AMIBERRY_SOURCE_KIND} ${AMIBERRY_SOURCE_REVISION} (${AMIBERRY_DISTRIBUTION})")
		return()
	endif()
endif()

file(WRITE "${OUT_FILE}" "${_amiberry_content}")
message(STATUS "Source offer: ${AMIBERRY_SOURCE_KIND} ${AMIBERRY_SOURCE_REVISION} (${AMIBERRY_DISTRIBUTION})")
