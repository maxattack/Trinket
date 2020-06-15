// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Common.h"
#include <EASTL/string.h>

// String-interning system, so the trinket can pass around
// hashes instead of strings, but still look-up the payload
// if necessary.

struct Name {
	size_t hash;

	Name() noexcept = default;
	Name(const Name&) noexcept = default;
	Name(Name&&) noexcept = default;
	Name& operator=(const Name&) noexcept = default;

	Name(ForceInit) noexcept : hash(0) {}
	Name(const char* cstring)  noexcept;
	explicit Name(size_t aHash) noexcept : hash(aHash) {}

	bool IsValid() const { return hash != 0; }
	eastl::string GetString() const;

	inline bool operator==(const Name& rhs) const { return hash == rhs.hash; }
	inline bool operator!=(const Name& rhs) const { return hash != rhs.hash; }

};
