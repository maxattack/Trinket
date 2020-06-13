#pragma once
#include "Common.h"
#include <EASTL/vector.h>
#include <algorithm>

// TODO: make a better "iterator" which accounts for events being added/removed mid-iteration

template<typename T>
class ListenerList {
private:
	eastl::vector<T*> listeners;

public:

	ListenerList() {}
	
	void TryAdd(T* listener) {
		let it = std::find(listeners.begin(), listeners.end(), listener);
		if (it == listeners.end())
			listeners.push_back(listener);
	}

	void TryRemove_Swap(T* listener) {
		let it = std::find(listeners.begin(), listeners.end(), listener);
		if (it != listeners.end()) {
			*it = listeners.back();
			listeners.pop_back();
		}
	}

	void TryRemove_Shift(T* listener) {
		let it = std::find(listeners.begin(), listeners.end(), listener);
		if (it != listeners.end())
			listeners.erase(it);
	}

	auto begin() { return listeners.begin(); }
	auto end() { return listeners.end(); }
};
