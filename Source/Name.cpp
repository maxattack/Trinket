// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

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

Name::Name(const char* cstring) noexcept
	: hash(eastl::hash<const char*>()(cstring))
{
	auto& lookup = GetNameLookup();

	lookup.mutex.lock_shared();
	let notInterned = lookup.map.find(hash) == lookup.map.end();
	lookup.mutex.unlock_shared();

	if (notInterned) {

		// pedant note: it's possible that the name was added after the first lock
		// was released and this second lock is taken -- however even in that case
		// all we're doing is double-writing the same value, which still works.

		// TODO: add testing-checks for name-hash collisions?

		lookup.mutex.lock();
		lookup.map[hash] = cstring;
		lookup.mutex.unlock();
	}
}

eastl::string Name::GetString() const {
	eastl::string result;
	if (hash)
	{
		auto& lookup = GetNameLookup();
		lookup.mutex.lock_shared();
		let it = lookup.map.find(hash);
		if (it != lookup.map.end())
			result = it->second;
		lookup.mutex.unlock_shared();
	}
	return result;
}

