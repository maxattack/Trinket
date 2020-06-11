#pragma once
#include "Common.h"
#include <EASTL/string.h>

// String-interning system, so the trinket can pass around
// hashes instead of strings, but still look-up the payload
// if necessary.

struct Name {
	size_t hash;
	uint32 suffix;

	Name() = default;
	Name(const Name&) = default;
	Name(Name&&) = default;
	Name& operator=(const Name&) = default;

	Name(ForceInit) noexcept : hash(0), suffix(0) {}
	Name(const char* cstring, uint32 aSuffix = 0)  noexcept;
	explicit Name(size_t aHash, uint32 aSuffix = 0) noexcept : hash(aHash), suffix(aSuffix) {}

	bool HasSuffix() const { return suffix != 0; }
	bool IsNumeric() const { return hash == 0 && suffix != 0; }
	bool IsValid() const { return hash != 0 || suffix != 0; }

	eastl::string GetString_NoSuffix() const;
	eastl::string GetString() const;

	inline bool operator==(const Name& rhs) const { return hash == rhs.hash && suffix == rhs.suffix; }

};
