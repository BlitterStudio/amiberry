// Edit sessions of the shared settings layer (amiberry_settings.h).

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "target.h"

#include "settings_definitions.h"

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
struct amiberry_settings_session {
	uae_prefs* working = nullptr;
	uae_prefs* base = nullptr;
	std::vector<bool> touched;
};

namespace {

uae_prefs* allocate_copy(const uae_prefs& source)
{
	auto* copy = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
	if (copy)
		std::memcpy(static_cast<void*>(copy), static_cast<const void*>(&source), sizeof(uae_prefs));
	return copy;
}

void copy_into(uae_prefs& to, const uae_prefs& from)
{
	std::memcpy(static_cast<void*>(&to), static_cast<const void*>(&from), sizeof(uae_prefs));
}

context current_context()
{
	return context{emulating != 0};
}

const definition* lookup(const char* id)
{
	return id ? amiberry_settings::find(id) : nullptr;
}

// The pending configuration with the session's changes applied, so changes
// made elsewhere since the session last read it are resolved together with
// the session's. NULL when memory is exhausted.
uae_prefs* merge_with_pending(const amiberry_settings_session& session)
{
	uae_prefs* merged = allocate_copy(changed_prefs);
	if (merged) {
		for (size_t i = 0; i < amiberry_settings::domain_count(); i++)
			amiberry_settings::domain_at(i).commit(*session.base, *session.working, *merged);
	}
	return merged;
}

bool same_value(const definition& d, const uae_prefs& a, const uae_prefs& b)
{
	// Full length, so two paths that share a prefix still differ.
	return amiberry_settings::values_equal(d, a, b);
}

} // namespace

extern "C" {

// Once a session exists the option index is built, so the other session
// calls cannot throw: they check the session before looking options up.
amiberry_settings_session* amiberry_settings_session_open(void)
{
	auto* session = new (std::nothrow) amiberry_settings_session;
	if (!session)
		return nullptr;
	session->working = allocate_copy(changed_prefs);
	session->base = allocate_copy(changed_prefs);
	const bool sized = session->working && session->base && amiberry_settings::no_throw(false, [session] {
		session->touched.assign(amiberry_settings::definition_count(), false);
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
	std::free(session->working);
	std::free(session->base);
	delete session;
}

amiberry_settings_status amiberry_settings_get(const amiberry_settings_session* session,
	const char* id, char* buffer, size_t size)
{
	const definition* d = session ? lookup(id) : nullptr;
	if (!d)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	return amiberry_settings::get(*d, *session->working, buffer, size);
}

amiberry_settings_status amiberry_settings_set(amiberry_settings_session* session,
	const char* id, const char* value)
{
	const definition* d = session ? lookup(id) : nullptr;
	if (!d)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (!value)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	const amiberry_settings_status status = amiberry_settings::set(*d, *session->working, current_context(), value);
	if (status == AMIBERRY_SETTINGS_OK)
		session->touched[amiberry_settings::position_of(*d)] = true;
	return status;
}

bool amiberry_settings_is_available(const amiberry_settings_session* session, const char* id)
{
	const definition* d = session ? lookup(id) : nullptr;
	return d && amiberry_settings::is_available(*d, *session->working, current_context());
}

bool amiberry_settings_choice_is_available(const amiberry_settings_session* session,
	const char* id, const char* value)
{
	const definition* d = session ? lookup(id) : nullptr;
	return d && value
		&& amiberry_settings::is_choice_available(*d, *session->working, current_context(), value);
}

bool amiberry_settings_get_range(const amiberry_settings_session* session, const char* id,
	double* min, double* max)
{
	const definition* d = session ? lookup(id) : nullptr;
	if (!d || !min || !max)
		return false;
	if (d->descriptor.type != AMIBERRY_SETTING_INT && d->descriptor.type != AMIBERRY_SETTING_REAL)
		return false;
	amiberry_settings::get_range(*d, *session->working, *min, *max);
	return true;
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
	fixup_prefs_preview(resolved);

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
	uae_prefs* resolved = session ? merge_with_pending(*session) : nullptr;
	if (resolved) {
		// Store what validation promised: the session's changes merged with
		// the current pending configuration and resolved by the core's
		// fixups, including what they adjust in other domains (24-bit
		// addressing removes Z3 RAM). resolved is changed_prefs as it is
		// now plus exactly those changes, so copying it whole reverts
		// nothing made elsewhere.
		fixup_prefs_preview(resolved);
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
			copy_into(changed_prefs, *resolved);
		}
		copy_into(*session->working, *resolved);
		copy_into(*session->base, *resolved);
		session->touched.assign(session->touched.size(), false);
		std::free(resolved);
	}
	if (changed)
		*changed = any;
	return strongest;
}


amiberry_settings_status amiberry_settings_preset_apply(amiberry_settings_session* session,
	const char* id, int config, int compatibility)
{
	if (!session)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	return amiberry_settings::apply_preset_checked(*session->working, id, config, compatibility);
}
}
