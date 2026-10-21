// Edit sessions of the shared settings layer (amiberry_settings.h).

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "target.h"

#include "settings_definitions.h"

#include <cstdlib>
#include <cstring>

using amiberry_settings::context;
using amiberry_settings::definition;

// The working copy is a byte copy of changed_prefs, the way the core copies
// prefs (copy_prefs). Its pointer members still point into changed_prefs,
// so it is never freed field by field, and only the scalar fields the
// domains own are written back. base is the pending configuration as the
// session last saw it; commit writes only what the session changed since.
struct amiberry_settings_session {
	uae_prefs* working;
	uae_prefs* base;
};

namespace {

uae_prefs* allocate_copy(const uae_prefs& source)
{
	auto* copy = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
	if (copy)
		std::memcpy(static_cast<void*>(copy), static_cast<const void*>(&source), sizeof(uae_prefs));
	return copy;
}

context current_context()
{
	return context{emulating != 0};
}

const definition* lookup(const char* id)
{
	return id ? amiberry_settings::find(id) : nullptr;
}

} // namespace

extern "C" {

amiberry_settings_session* amiberry_settings_session_open(void)
{
	auto* session = static_cast<amiberry_settings_session*>(std::malloc(sizeof(amiberry_settings_session)));
	if (!session)
		return nullptr;
	session->working = allocate_copy(changed_prefs);
	session->base = allocate_copy(changed_prefs);
	if (!session->working || !session->base) {
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
	std::free(session);
}

amiberry_settings_status amiberry_settings_get(const amiberry_settings_session* session,
	const char* id, char* buffer, size_t size)
{
	const definition* d = lookup(id);
	if (!session || !d)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	return amiberry_settings::get(*d, *session->working, buffer, size);
}

amiberry_settings_status amiberry_settings_set(amiberry_settings_session* session,
	const char* id, const char* value)
{
	const definition* d = lookup(id);
	if (!session || !d)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (!value)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	return amiberry_settings::set(*d, *session->working, current_context(), value);
}

bool amiberry_settings_is_available(const amiberry_settings_session* session, const char* id)
{
	const definition* d = lookup(id);
	return session && d && amiberry_settings::is_available(*d, *session->working, current_context());
}

bool amiberry_settings_choice_is_available(const amiberry_settings_session* session,
	const char* id, const char* value)
{
	const definition* d = lookup(id);
	return session && d && value
		&& amiberry_settings::is_choice_available(*d, *session->working, current_context(), value);
}

bool amiberry_settings_get_range(const amiberry_settings_session* session, const char* id,
	double* min, double* max)
{
	const definition* d = lookup(id);
	if (!session || !d || !min || !max)
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
	uae_prefs* resolved = allocate_copy(*session->working);
	if (!resolved)
		return 0;
	fixup_prefs_preview(resolved);

	size_t found = 0;
	for (size_t i = 0; i < amiberry_settings::definition_count(); i++) {
		const definition& d = amiberry_settings::definition_at(i);
		char requested[AMIBERRY_SETTINGS_VALUE_MAX];
		char adjusted[AMIBERRY_SETTINGS_VALUE_MAX];
		d.get(*session->working, requested, sizeof requested);
		d.get(*resolved, adjusted, sizeof adjusted);
		if (std::strcmp(requested, adjusted) == 0)
			continue;
		if (adjustments && found < capacity) {
			amiberry_settings_adjustment& a = adjustments[found];
			a.id = d.descriptor.id;
			std::memcpy(a.requested, requested, sizeof requested);
			std::memcpy(a.adjusted, adjusted, sizeof adjusted);
			a.reason = d.descriptor.constraint ? d.descriptor.constraint
				: "Changed by the emulator's configuration checks.";
		}
		found++;
	}
	std::free(resolved);
	return found;
}

amiberry_apply_class amiberry_settings_commit(amiberry_settings_session* session, bool* changed)
{
	amiberry_apply_class strongest = AMIBERRY_APPLY_LIVE;
	bool any = false;
	if (session) {
		// Store what validation promised: the values after the core's fixups.
		fixup_prefs_preview(session->working);
		for (size_t i = 0; i < amiberry_settings::definition_count(); i++) {
			const definition& d = amiberry_settings::definition_at(i);
			char base[AMIBERRY_SETTINGS_VALUE_MAX];
			char edited[AMIBERRY_SETTINGS_VALUE_MAX];
			char pending[AMIBERRY_SETTINGS_VALUE_MAX];
			d.get(*session->base, base, sizeof base);
			d.get(*session->working, edited, sizeof edited);
			d.get(changed_prefs, pending, sizeof pending);
			// Only what the session changed is written, and it only counts
			// when the pending configuration does not already hold it.
			if (std::strcmp(base, edited) == 0 || std::strcmp(pending, edited) == 0)
				continue;
			any = true;
			if (d.descriptor.apply > strongest)
				strongest = d.descriptor.apply;
		}
		for (size_t i = 0; i < amiberry_settings::domain_count(); i++)
			amiberry_settings::domain_at(i).commit(*session->base, *session->working, changed_prefs);
		std::memcpy(static_cast<void*>(session->base), static_cast<const void*>(session->working), sizeof(uae_prefs));
	}
	if (changed)
		*changed = any;
	return strongest;
}

}
