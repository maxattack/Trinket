#include "Name.h"
#include <EASTL/functional.h>
#include <EASTL/hash_map.h>
#include <shared_mutex>

static auto& GetNameLookup() {
	struct Intern {
		std::shared_mutex mutex;
		eastl::hash_map<size_t, eastl::string> map;
	};

	static Intern intern;
	return intern;
}

Name::Name(const char* cstring, uint32 aSuffix) noexcept
	: hash(eastl::hash<const char*>()(cstring))
	, suffix(aSuffix)
{
	auto& lookup = GetNameLookup();

	lookup.mutex.lock_shared();
	let notInterned = lookup.map.find(hash) == lookup.map.end();
	lookup.mutex.unlock_shared();

	if (notInterned) {
		lookup.mutex.lock();
		lookup.map[hash] = cstring;
		lookup.mutex.unlock();
	}
}

eastl::string Name::GetString_NoSuffix() const {
	eastl::string result;
	if (hash == 0)
		return result;

	auto& lookup = GetNameLookup();
	lookup.mutex.lock_shared();
	let it = lookup.map.find(hash);
	if (it != lookup.map.end())
		result = it->second;
	lookup.mutex.unlock_shared();

	return result;
}

eastl::string Name::GetString() const {
	eastl::string result;

	if (hash == 0)
		return suffix == 0 ? result : eastl::to_string(suffix);

	auto& lookup = GetNameLookup();
	lookup.mutex.lock_shared();
	let it = lookup.map.find(hash);
	if (it != lookup.map.end())
		result = it->second;
	lookup.mutex.unlock_shared();

	return suffix == 0 ? result : result + eastl::to_string(suffix);
}

