#pragma once

// Null/empty-safe display name for expansion board entries.
//
// The expansion database (expansionroms[] in expansion.cpp) declares each
// board's friendlyname/friendlymanufacturer as static data, and not every
// board has either: entries with a NULL or empty friendlyname used to reach
// the sort comparator, combo previews and label builders in the GUI, crashing
// on the NULL dereference. Every site that renders or compares a board name
// goes through this helper instead of reading friendlyname raw: it returns
// the friendly name when it is a usable string, the manufacturer when only
// that exists, and a generic placeholder otherwise, so the returned pointer
// is never NULL and never empty.
//
// The helper takes the two name fields rather than the board entry so it
// stays free of database headers: core files (expansion.cpp), osdep GUI
// files and the standalone tests can all include it. TCHAR is char on every
// Amiberry platform; the wchar_t overload keeps the helper usable verbatim
// where TCHAR is wide.

inline const char* expansion_display_name(const char* friendlyname, const char* friendlymanufacturer)
{
	if (friendlyname && friendlyname[0])
		return friendlyname;
	if (friendlymanufacturer && friendlymanufacturer[0])
		return friendlymanufacturer;
	return "Unknown board";
}

inline const wchar_t* expansion_display_name(const wchar_t* friendlyname, const wchar_t* friendlymanufacturer)
{
	if (friendlyname && friendlyname[0])
		return friendlyname;
	if (friendlymanufacturer && friendlymanufacturer[0])
		return friendlymanufacturer;
	return L"Unknown board";
}
