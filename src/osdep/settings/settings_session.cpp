// Edit sessions of the shared settings layer (amiberry_settings.h).

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "target.h"
#include "autoconf.h"

#include "settings_definitions.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

using amiberry_settings::context;
using amiberry_settings::definition;

// working is the session's edited copy and base the pending configuration
// (changed_prefs) as the session last read or wrote it; the difference is
// what the session changed. Both are byte copies, the way the core copies
// prefs (copy_prefs): their pointer members still point into changed_prefs,
// so they are never freed field by field, and only the scalar fields the
// domains own are written back. touched marks the options the caller set.
//
// A preset replaces far more than the options (the target defaults of a
// fresh configuration), so it is not merged field by field: the session
// records it and replays it on the pending configuration the way Quickstart
// applies it. pre_preset is working just before the last preset (its
// changes from base are replayed first), after_preset working just after it
// (changes made since are replayed on top).
struct amiberry_settings_session {
	uae_prefs* working = nullptr;
	uae_prefs* base = nullptr;
	std::vector<bool> touched;
	int preset_model = -1; // none
	int preset_config = 0;
	int preset_compatibility = 0;
	uae_prefs* pre_preset = nullptr;
	uae_prefs* after_preset = nullptr;
};

namespace {

// Every open session, so a host device refresh can find the devices their
// configurations select again (for_each_session_config). Main thread only,
// like every session call.
std::vector<amiberry_settings_session*>& open_sessions()
{
	static std::vector<amiberry_settings_session*> sessions;
	return sessions;
}

// A board ROM entry points back at its board (romconfig::back), which the
// core follows from currprefs (expansion sorting, savestates). A byte copy
// leaves those pointers where they were: a board a session added would point
// into the session's freed copy once it reached changed_prefs. Every
// configuration the layer copies points them at changed_prefs' board in
// the same slot instead, the object that outlives sessions (and what
// currprefs points at too), so copies of unchanged boards stay byte-equal.
void rebind_board_roms(uae_prefs& p)
{
	for (size_t i = 0; i < MAX_EXPANSION_BOARDS; i++) {
		for (romconfig& rom : p.expansionboard[i].roms) {
			if (rom.back)
				rom.back = &changed_prefs.expansionboard[i];
		}
	}
}

uae_prefs* allocate_copy(const uae_prefs& source)
{
	auto* copy = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
	if (copy) {
		std::memcpy(static_cast<void*>(copy), static_cast<const void*>(&source), sizeof(uae_prefs));
		rebind_board_roms(*copy);
	}
	return copy;
}

void copy_into(uae_prefs& to, const uae_prefs& from)
{
	std::memcpy(static_cast<void*>(&to), static_cast<const void*>(&from), sizeof(uae_prefs));
	rebind_board_roms(to);
}

context current_context()
{
	return context{emulating != 0};
}

const definition* lookup(const char* id)
{
	return id ? amiberry_settings::find(id) : nullptr;
}

void commit_domains(const amiberry_settings_session& session, const uae_prefs& base,
	const uae_prefs& from, uae_prefs& to)
{
	const amiberry_settings::commit_info info{session.touched};
	for (size_t i = 0; i < amiberry_settings::domain_count(); i++)
		amiberry_settings::domain_at(i).commit(base, from, to, info);
}

void forget_preset(amiberry_settings_session& session)
{
	std::free(session.pre_preset);
	std::free(session.after_preset);
	session.pre_preset = session.after_preset = nullptr;
	session.preset_model = -1;
}

// The pending configuration with the session's changes applied, so changes
// made elsewhere since the session last read it are resolved together with
// the session's. A recorded preset is replayed between the changes made
// before and after it. NULL when memory is exhausted (the preset's target
// defaults use std::string).
uae_prefs* merge_with_pending(const amiberry_settings_session& session)
{
	uae_prefs* merged = allocate_copy(changed_prefs);
	if (!merged)
		return nullptr;
	if (session.preset_model < 0) {
		commit_domains(session, *session.base, *session.working, *merged);
		return merged;
	}
	commit_domains(session, *session.base, *session.pre_preset, *merged);
	const bool applied = amiberry_settings::no_throw(false, [&session, merged] {
		amiberry_settings::apply_preset(*merged, session.preset_model, session.preset_config,
			session.preset_compatibility, true);
		return true;
	});
	if (!applied) {
		std::free(merged);
		return nullptr;
	}
	commit_domains(session, *session.after_preset, *session.working, *merged);
	return merged;
}

// fixup_prefs_preview() on p; false when it threw (parts of the core's
// fixups and target options use std::string).
bool resolve_preview(uae_prefs* p)
{
	return amiberry_settings::no_throw(false, [p] {
		fixup_prefs_preview(p);
		return true;
	});
}

bool same_value(const definition& d, const uae_prefs& a, const uae_prefs& b)
{
	// Full length, so two paths that share a prefix still differ.
	return amiberry_settings::values_equal(d, a, b);
}

// Removes mount entry index from p without touching the running emulation
// (kill_filesys_unitconfig() also detaches the drive): the session only
// edits a configuration.
void remove_mount(uae_prefs& p, int index)
{
	const int last = p.mountitems - 1;
	std::memmove(static_cast<void*>(&p.mountconfig[index]), static_cast<const void*>(&p.mountconfig[index + 1]),
		static_cast<size_t>(last - index) * sizeof p.mountconfig[0]);
	std::memset(static_cast<void*>(&p.mountconfig[last]), 0, sizeof p.mountconfig[last]);
	p.mountconfig[last].configoffset = -1;
	p.mountconfig[last].unitnum = -1;
	p.mountitems = last;
}

// Adds value to p the way the hard drive panel adds an entry
// (add_filesys_config(), which refuses a device name in use or a
// controller without a free unit), at the end or, when index is not
// negative, in place of entry index. Leaves p alone unless it returns OK.
amiberry_settings_status put_mount(uae_prefs& p, int index, const char* value)
{
	uaedev_config_info ci{};
	if (!value || !cfgfile_parse_mount_value(&p, value, &ci))
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	// The name the entry will have (add_filesys_config() shortens and
	// cleans it only after its own duplicate check), compared without
	// case as AmigaDOS compares device names.
	validatedevicename(ci.devname, nullptr);
	if (ci.devname[0]) {
		for (int i = 0; i < p.mountitems && i < MOUNT_CONFIG_SIZE; i++) {
			if (i != index && !_tcsicmp(p.mountconfig[i].ci.devname, ci.devname))
				return AMIBERRY_SETTINGS_INVALID_VALUE;
		}
	}
	uae_prefs* scratch = allocate_copy(p);
	if (!scratch)
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	uaedev_config_data replaced{};
	if (index >= 0) {
		replaced = scratch->mountconfig[index];
		// What a uaehfN value does not carry stays with the entry.
		if (ci.type == UAEDEV_DIR && replaced.ci.type == UAEDEV_DIR)
			ci.inject_icons = replaced.ci.inject_icons;
		remove_mount(*scratch, index);
	}
	if (!add_filesys_config(scratch, -1, &ci)) {
		std::free(scratch);
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	}
	const int added = scratch->mountitems - 1;
	if (index >= 0) {
		// Back in the replaced entry's place, attached to the same unit.
		uaedev_config_data entry = scratch->mountconfig[added];
		std::memmove(static_cast<void*>(&scratch->mountconfig[index + 1]),
			static_cast<const void*>(&scratch->mountconfig[index]),
			static_cast<size_t>(added - index) * sizeof scratch->mountconfig[0]);
		entry.configoffset = replaced.configoffset;
		entry.unitnum = replaced.unitnum;
		scratch->mountconfig[index] = entry;
	}
	// An entry whose line the loader would cut short would not survive a
	// save and a load.
	if (!cfgfile_mount_entry_fits(scratch, index >= 0 ? index : added)) {
		std::free(scratch);
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	}
	copy_into(p, *scratch);
	std::free(scratch);
	return AMIBERRY_SETTINGS_OK;
}

bool valid_mount_index(const amiberry_settings_session* session, size_t index)
{
	return session && index < static_cast<size_t>(session->working->mountitems)
		&& index < MOUNT_CONFIG_SIZE;
}

} // namespace

extern "C" {

// Every entry point returns its error value instead of letting a C++
// exception (std::bad_alloc from a setter, a hook or the core's fixups)
// cross into C, Swift, C# or Kotlin callers.
amiberry_settings_session* amiberry_settings_session_open(void)
{
	auto* session = new (std::nothrow) amiberry_settings_session;
	if (!session)
		return nullptr;
	// changed_prefs' own boards may point elsewhere (a configuration loaded
	// into another prefs and copied); normalised, the session's copies
	// compare equal to it until something changes.
	rebind_board_roms(changed_prefs);
	session->working = allocate_copy(changed_prefs);
	session->base = allocate_copy(changed_prefs);
	const bool sized = session->working && session->base && amiberry_settings::no_throw(false, [session] {
		session->touched.assign(amiberry_settings::definition_count(), false);
		open_sessions().push_back(session);
		return true;
	});
	if (!sized) {
		amiberry_settings_session_close(session);
		return nullptr;
	}
	return session;
}

void amiberry_settings_session_close(amiberry_settings_session* session)
{
	if (!session)
		return;
	auto& sessions = open_sessions();
	sessions.erase(std::remove(sessions.begin(), sessions.end(), session), sessions.end());
	forget_preset(*session);
	std::free(session->working);
	std::free(session->base);
	delete session;
}

amiberry_settings_status amiberry_settings_get(const amiberry_settings_session* session,
	const char* id, char* buffer, size_t size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		const definition* d = session ? lookup(id) : nullptr;
		if (!d)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		return amiberry_settings::get(*d, *session->working, buffer, size);
	});
}

amiberry_settings_status amiberry_settings_get_active(const char* id, char* buffer, size_t size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		const definition* d = lookup(id);
		if (!d)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		return amiberry_settings::get(*d, currprefs, buffer, size);
	});
}

amiberry_settings_status amiberry_settings_set(amiberry_settings_session* session,
	const char* id, const char* value)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNAVAILABLE, [=] {
		const definition* d = session ? lookup(id) : nullptr;
		if (!d)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		if (!value)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		const amiberry_settings_status status = amiberry_settings::set(*d, *session->working, current_context(), value);
		if (status == AMIBERRY_SETTINGS_OK)
			session->touched[amiberry_settings::position_of(*d)] = true;
		return status;
	});
}

bool amiberry_settings_is_available(const amiberry_settings_session* session, const char* id)
{
	return amiberry_settings::no_throw(false, [=] {
		const definition* d = session ? lookup(id) : nullptr;
		return d && amiberry_settings::is_available(*d, *session->working, current_context());
	});
}

bool amiberry_settings_choice_is_available(const amiberry_settings_session* session,
	const char* id, const char* value)
{
	return amiberry_settings::no_throw(false, [=] {
		const definition* d = session ? lookup(id) : nullptr;
		return d && value
			&& amiberry_settings::is_choice_available(*d, *session->working, current_context(), value);
	});
}

bool amiberry_settings_get_range(const amiberry_settings_session* session, const char* id,
	double* min, double* max)
{
	return amiberry_settings::no_throw(false, [=] {
		const definition* d = session ? lookup(id) : nullptr;
		if (!d || !min || !max)
			return false;
		if (d->descriptor.type != AMIBERRY_SETTING_INT && d->descriptor.type != AMIBERRY_SETTING_REAL)
			return false;
		amiberry_settings::get_range(*d, *session->working, *min, *max);
		return true;
	});
}

size_t amiberry_settings_choices(const amiberry_settings_session* session,
	const char* id, const amiberry_setting_choice** choices)
{
	const definition* d = session ? lookup(id) : nullptr;
	if (!d || !choices)
		return 0;
	return amiberry_settings::no_throw<size_t>(0, [&] {
		const amiberry_settings::choice_list list =
			amiberry_settings::choices_of(*d, *session->working);
		*choices = list.items;
		return list.count;
	});
}
void amiberry_settings_refresh_choices(void)
{
	amiberry_settings::no_throw(0, [] {
		amiberry_settings::refresh_dynamic_choices();
		return 0;
	});
}

size_t amiberry_settings_validate(const amiberry_settings_session* session,
	amiberry_settings_adjustment* adjustments, size_t capacity)
{
	if (!session)
		return 0;
	uae_prefs* requested = merge_with_pending(*session);
	uae_prefs* resolved = requested ? allocate_copy(*requested) : nullptr;
	if (!resolved) {
		std::free(requested);
		return 0;
	}
	if (!resolve_preview(resolved)) {
		std::free(requested);
		std::free(resolved);
		return 0;
	}

	size_t found = 0;
	for (size_t i = 0; i < amiberry_settings::definition_count(); i++) {
		const definition& d = amiberry_settings::definition_at(i);
		if (same_value(d, *requested, *resolved))
			continue;
		if (adjustments && found < capacity) {
			amiberry_settings_adjustment& a = adjustments[found];
			std::memset(&a, 0, sizeof a);
			a.id = d.descriptor.id;
			d.get(*requested, a.requested, sizeof a.requested);
			d.get(*resolved, a.adjusted, sizeof a.adjusted);
			a.reason = d.descriptor.constraint ? d.descriptor.constraint
				: "Changed by the emulator's configuration checks.";
		}
		found++;
	}
	std::free(requested);
	std::free(resolved);
	return found;
}

amiberry_apply_class amiberry_settings_commit(amiberry_settings_session* session, bool* changed)
{
	amiberry_apply_class strongest = AMIBERRY_APPLY_LIVE;
	bool any = false;
	// As at open: prefs byte-copied into changed_prefs since then may have
	// brought their own board pointers.
	rebind_board_roms(changed_prefs);
	uae_prefs* resolved = session ? merge_with_pending(*session) : nullptr;
	if (resolved) {
		// Store what validation promised: the session's changes merged with
		// the current pending configuration and resolved by the core's
		// fixups, including what they adjust in other domains (24-bit
		// addressing removes Z3 RAM). resolved is changed_prefs as it is
		// now plus exactly those changes, so copying it whole reverts
		// nothing made elsewhere.
		if (!resolve_preview(resolved)) {
			std::free(resolved);
			if (changed)
				*changed = false;
			return AMIBERRY_APPLY_LIVE;
		}
		any = std::memcmp(static_cast<const void*>(&changed_prefs), static_cast<const void*>(resolved),
			sizeof(uae_prefs)) != 0;
		if (any) {
			bool value_changed = false;
			for (size_t i = 0; i < amiberry_settings::definition_count(); i++) {
				const definition& d = amiberry_settings::definition_at(i);
				if (!same_value(d, changed_prefs, *resolved)) {
					value_changed = true;
					if (d.descriptor.apply > strongest)
						strongest = d.descriptor.apply;
				}
			}
			// Only fields without a value of their own changed (re-selecting
			// a JIT memory access mode aligns the other modes): the options
			// the caller set account for them.
			if (!value_changed) {
				for (size_t i = 0; i < amiberry_settings::definition_count(); i++) {
					const definition& d = amiberry_settings::definition_at(i);
					if (session->touched[i] && d.descriptor.apply > strongest)
						strongest = d.descriptor.apply;
				}
			}
			// A preset is a machine change, like Quickstart's.
			if (session->preset_model >= 0 && AMIBERRY_APPLY_RESET > strongest)
				strongest = AMIBERRY_APPLY_RESET;
			// Drives are attached when the emulation resets.
			if (!amiberry_settings::same_mount_list(changed_prefs, *resolved) && AMIBERRY_APPLY_RESET > strongest)
				strongest = AMIBERRY_APPLY_RESET;
			// Boards are built when the emulation starts.
			if (!amiberry_settings::same_boards(changed_prefs, *resolved) && AMIBERRY_APPLY_RESTART > strongest)
				strongest = AMIBERRY_APPLY_RESTART;
			copy_into(changed_prefs, *resolved);
		}
		copy_into(*session->working, *resolved);
		copy_into(*session->base, *resolved);
		session->touched.assign(session->touched.size(), false);
		forget_preset(*session);
		std::free(resolved);
	}
	if (changed)
		*changed = any;
	return strongest;
}

size_t amiberry_settings_mount_count(const amiberry_settings_session* session)
{
	return session ? static_cast<size_t>(session->working->mountitems) : 0;
}

amiberry_settings_status amiberry_settings_mount_get(const amiberry_settings_session* session,
	size_t index, char* buffer, size_t size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		if (!valid_mount_index(session, index))
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		TCHAR value[CONFIG_BLEN];
		if (!cfgfile_mount_value(session->working, static_cast<int>(index), value, sizeof value / sizeof value[0]))
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		const size_t length = std::strlen(value);
		if (!buffer || length >= size)
			return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
		std::memcpy(buffer, value, length + 1);
		return AMIBERRY_SETTINGS_OK;
	});
}

amiberry_settings_status amiberry_settings_mount_add(amiberry_settings_session* session, const char* value)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNAVAILABLE, [=] {
		if (!session)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		if (session->working->mountitems >= MOUNT_CONFIG_SIZE)
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		return put_mount(*session->working, -1, value);
	});
}

amiberry_settings_status amiberry_settings_mount_replace(amiberry_settings_session* session,
	size_t index, const char* value)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNAVAILABLE, [=] {
		if (!valid_mount_index(session, index))
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		return put_mount(*session->working, static_cast<int>(index), value);
	});
}

amiberry_settings_status amiberry_settings_mount_remove(amiberry_settings_session* session, size_t index)
{
	if (!valid_mount_index(session, index))
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	remove_mount(*session->working, static_cast<int>(index));
	return AMIBERRY_SETTINGS_OK;
}

// The board catalog is built on first use (std::vector, std::string);
// nothing may throw across the C API.
size_t amiberry_settings_board_type_count(void)
{
	return amiberry_settings::no_throw(size_t{0}, [] { return amiberry_settings::board_type_count(); });
}

const amiberry_settings_board_type* amiberry_settings_board_type_at(size_t index)
{
	return amiberry_settings::no_throw(static_cast<const amiberry_settings_board_type*>(nullptr),
		[index] { return amiberry_settings::board_type_at(index); });
}

const amiberry_settings_board_type* amiberry_settings_board_type_find(const char* id)
{
	return amiberry_settings::no_throw(static_cast<const amiberry_settings_board_type*>(nullptr),
		[id] { return id ? amiberry_settings::board_type_find(id) : nullptr; });
}

const amiberry_setting_descriptor* amiberry_settings_board_field_at(const amiberry_settings_board_type* type,
	size_t index)
{
	return amiberry_settings::no_throw(static_cast<const amiberry_setting_descriptor*>(nullptr),
		[type, index] { return amiberry_settings::board_field_at(type, index); });
}

amiberry_settings_status amiberry_settings_board_get(const amiberry_settings_session* session, const char* type,
	int unit, const char* field, char* buffer, size_t size)
{
	if (!session || !type || !field)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		return amiberry_settings::board_get(*session->working, type, unit, field, buffer, size);
	});
}

amiberry_settings_status amiberry_settings_board_set(amiberry_settings_session* session, const char* type,
	int unit, const char* field, const char* value)
{
	if (!session || !type || !field)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (!value)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		return amiberry_settings::board_set(*session->working, current_context(), type, unit, field, value);
	});
}

bool amiberry_settings_board_field_available(const amiberry_settings_session* session, const char* type,
	int unit, const char* field)
{
	if (!session || !type || !field)
		return false;
	return amiberry_settings::no_throw(false, [=] {
		return amiberry_settings::board_field_available(*session->working, current_context(), type, unit, field);
	});
}

// The preset is applied to the session's copy for get/is_available, and
// recorded so validate and commit replay it on the pending configuration.
amiberry_settings_status amiberry_settings_preset_apply(amiberry_settings_session* session,
	const char* id, int config, int compatibility)
{
	if (!session)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	int model = -1;
	const amiberry_settings_status status = amiberry_settings::check_preset(id, config, compatibility, model);
	if (status != AMIBERRY_SETTINGS_OK)
		return status;
	uae_prefs* pre = allocate_copy(*session->working);
	uae_prefs* after = pre ? allocate_copy(*session->working) : nullptr;
	const bool applied = after && amiberry_settings::no_throw(false, [=] {
		amiberry_settings::apply_preset(*after, model, config, compatibility, true);
		return true;
	});
	if (!applied) {
		std::free(pre);
		std::free(after);
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	}
	forget_preset(*session);
	session->pre_preset = pre;
	session->after_preset = after;
	session->preset_model = model;
	session->preset_config = config;
	session->preset_compatibility = compatibility;
	copy_into(*session->working, *after);
	return AMIBERRY_SETTINGS_OK;
}
}

namespace amiberry_settings {

void for_each_session_config(void (*visit)(uae_prefs& p, void* context), void* context)
{
	for (amiberry_settings_session* session : open_sessions()) {
		for (uae_prefs* p : {session->working, session->base, session->pre_preset, session->after_preset}) {
			if (p)
				visit(*p, context);
		}
	}
}

} // namespace amiberry_settings
