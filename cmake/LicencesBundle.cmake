# Copies the licences catalogue into an app bundle at BUILD time.
#
# Runs from an always-executed custom target, so the copy refreshes when the
# catalogue, a licence text or the generated source offer changes - even when
# the executable does not relink - and it runs before the POST_BUILD codesign.
#
# The destination is rebuilt from scratch (renamed or deleted texts never
# linger) and only the texts this distribution's catalogue entries use are
# copied: a Mac App Store bundle never receives, say, CAPSImage.txt.

file(REMOVE_RECURSE "${DEST_DIR}")
file(MAKE_DIRECTORY "${DEST_DIR}/texts")

file(READ "${SRC_DIR}/catalogue.json" _catalogue)

string(JSON _count LENGTH "${_catalogue}" "components")
set(_i 0)
set(_texts "")
while(_i LESS _count)
	string(JSON _text GET "${_catalogue}" "components" ${_i} "text")
	if(NOT _text STREQUAL "")
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
			list(APPEND _texts "${_text}")
		endif()
	endif()
	math(EXPR _i "${_i} + 1")
endwhile()

list(REMOVE_DUPLICATES _texts)
foreach(_text IN LISTS _texts)
	file(COPY "${SRC_DIR}/texts/${_text}" DESTINATION "${DEST_DIR}/texts")
endforeach()

file(COPY "${SRC_DIR}/catalogue.json" DESTINATION "${DEST_DIR}")
configure_file("${OFFER_FILE}" "${DEST_DIR}/source-offer.json" COPYONLY)
