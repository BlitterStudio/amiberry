# Copies the licences catalogue into an app bundle at BUILD time.
#
# Runs from an always-executed custom target, so the copy refreshes when the
# catalogue, a licence text or the generated source offer changes - even when
# the executable does not relink - and it runs before the POST_BUILD codesign.
#
# The bundle content is hashed first and the copy is skipped when nothing
# changed, so a clean tree is not rewritten (and macOS bundles are not
# re-signed, which would dirty the build) on every rebuild. When the copy
# does change a signed bundle, the macOS step re-signs it here: the
# executable's POST_BUILD codesign only runs on a relink.
#
# Only the texts this distribution's catalogue entries use are copied: a Mac
# App Store bundle never receives, say, CAPSImage.txt.

file(READ "${SRC_DIR}/catalogue.json" _catalogue)

string(JSON _count LENGTH "${_catalogue}" "components")
set(_i 0)
set(_texts "")
while(_i LESS _count)
	# "text" is one file name or a list: an entry can carry several
	# notices (for example libjpeg-turbo's BSD and IJG texts).
	string(JSON _text_type TYPE "${_catalogue}" "components" ${_i} "text")
	set(_entry_texts "")
	if(_text_type STREQUAL "STRING")
		string(JSON _text GET "${_catalogue}" "components" ${_i} "text")
		if(NOT _text STREQUAL "")
			list(APPEND _entry_texts "${_text}")
		endif()
	elseif(_text_type STREQUAL "ARRAY")
		string(JSON _texts_count LENGTH "${_catalogue}" "components" ${_i} "text")
		set(_k 0)
		while(_k LESS _texts_count)
			string(JSON _text GET "${_catalogue}" "components" ${_i} "text" ${_k})
			if(NOT _text STREQUAL "")
				list(APPEND _entry_texts "${_text}")
			endif()
			math(EXPR _k "${_k} + 1")
		endwhile()
	endif()
	if(NOT _entry_texts STREQUAL "")
		set(_platforms_count 0)
		string(JSON _platforms_count LENGTH "${_catalogue}" "components" ${_i} "platforms")
		set(_j 0)
		set(_ships OFF)
		while(_j LESS _platforms_count)
			string(JSON _platform GET "${_catalogue}" "components" ${_i} "platforms" ${_j})
			if(_platform STREQUAL "${DISTRIBUTION}")
				set(_ships ON)
			endif()
			math(EXPR _j "${_j} + 1")
		endwhile()
		if(_ships)
			list(APPEND _texts ${_entry_texts})
		endif()
	endif()
	math(EXPR _i "${_i} + 1")
endwhile()

list(REMOVE_DUPLICATES _texts)

# Skip everything when neither the catalogue, an offer nor any shipped text
# changed; a rewrite would re-sign and dirty every macOS rebuild.
set(_digest "${DISTRIBUTION}")
string(SHA256 _catalogue_hash "${_catalogue}")
string(APPEND _digest " ${_catalogue_hash}")
file(SHA256 "${OFFER_FILE}" _offer_hash)
string(APPEND _digest " ${_offer_hash}")
foreach(_text IN LISTS _texts)
	file(SHA256 "${SRC_DIR}/texts/${_text}" _text_hash)
	string(APPEND _digest " ${_text}:${_text_hash}")
endforeach()
string(SHA256 _bundle_hash "${_digest}")

# The stamp is keyed to the destination: configurations that share a build
# directory (Xcode Debug and Release) must not skip each other's copies,
# and a matching stamp only counts while the destination still holds what
# it recorded.
set(_digest_input "${DEST_DIR}")
string(SHA256 _dest_hash "${_digest_input}")
get_filename_component(_offer_dir "${OFFER_FILE}" DIRECTORY)
set(_stamp "${_offer_dir}/licences-${DISTRIBUTION}-${_dest_hash}.stamp")
if(EXISTS "${_stamp}"
		AND EXISTS "${DEST_DIR}/catalogue.json"
		AND EXISTS "${DEST_DIR}/source-offer.json"
		AND IS_DIRECTORY "${DEST_DIR}/texts")
	file(READ "${_stamp}" _existing_stamp)
	if(_existing_stamp STREQUAL _bundle_hash)
		message(STATUS "Licences bundle unchanged (${DISTRIBUTION})")
		return()
	endif()
endif()

file(REMOVE_RECURSE "${DEST_DIR}")
file(MAKE_DIRECTORY "${DEST_DIR}/texts")
foreach(_text IN LISTS _texts)
	file(COPY "${SRC_DIR}/texts/${_text}" DESTINATION "${DEST_DIR}/texts")
endforeach()
file(COPY "${SRC_DIR}/catalogue.json" DESTINATION "${DEST_DIR}")
configure_file("${OFFER_FILE}" "${DEST_DIR}/source-offer.json" COPYONLY)

# The copy changed an already-built, signed bundle. The executable's
# POST_BUILD codesign only runs on a relink, so sign here - but only when
# the executable exists: on a clean build this step runs before the app
# target and the first signing belongs to POST_BUILD.
if(CODESIGN_HELPER AND CODESIGN_APP AND IS_DIRECTORY "${CODESIGN_APP}/Contents/MacOS")
	execute_process(
		COMMAND "${CMAKE_COMMAND}"
			"-DAPP_BUNDLE=${CODESIGN_APP}"
			"-DCODESIGN_EXECUTABLE=${CODESIGN_EXECUTABLE}"
			"-DAMIBERRY_CODESIGN_IDENTITY=${CODESIGN_IDENTITY}"
			"-DAMIBERRY_CODESIGN_OPTIONS=${CODESIGN_OPTIONS}"
			"-DAMIBERRY_ENTITLEMENTS=${CODESIGN_ENTITLEMENTS}"
			-P "${CODESIGN_HELPER}"
		RESULT_VARIABLE _codesign_result)
	if(NOT _codesign_result EQUAL 0)
		message(FATAL_ERROR "Re-codesigning after the licences copy failed")
	endif()
endif()

file(WRITE "${_stamp}" "${_bundle_hash}")
message(STATUS "Licences bundle updated (${DISTRIBUTION})")
