// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Math.h"
#include <SDL.h>

struct GameTimestamp {
	float time;

	GameTimestamp() noexcept {}
	GameTimestamp(float aTime) noexcept : time(aTime) {}

	operator float() const { return time; }
	float operator-(GameTimestamp rhs) const { return time - rhs.time; }
};

struct RealTimestamp {
	uint32 ticks;

	RealTimestamp() noexcept {}
	RealTimestamp(uint32 aTicks) noexcept : ticks(aTicks) {}


	operator uint32() const { return ticks; }
	operator float() const { return ticks * 0.001f; }
	int32 operator-(RealTimestamp rhs) const { return ticks - rhs.ticks; }
};

class Input {
private:
	SDL_GameController* pGamepad = nullptr;
	// TODO secondary gamepads?

	uint32 ticks = 0;
	uint32 deltaTicks = 0;
	float time = 0.f;
	float deltaTime = 0.f;
	float dilation = 1.f;

	// TODO event listener interface?

public:

	Input();

	// Realtime:
	RealTimestamp GetTicks() const { return ticks; }
	int32 GetDeltaTicks() const { return deltaTicks; }

	// Dilated Gametime:
	GameTimestamp GetTime() const { return time; }
	float GetDeltaTime() const { return deltaTime; }
	float GetTimeDilation() const { return dilation; }
	void SetTimeDilation(float multi);

	// Time Helpers
	bool InPast(GameTimestamp stamp) const { return stamp.time < time; }
	bool InPast(RealTimestamp stamp) const { return stamp.ticks < ticks; }
	bool InFuture(GameTimestamp stamp) const { return stamp.time > time; }
	bool InFuture(RealTimestamp stamp) const { return stamp.ticks > ticks; }

	// Gamepad Analog Inputs:
	float GetStickLX() const;
	float GetStickLY() const;
	float GetStickRX() const;
	float GetStickRY() const;
	float GetTriggerValueL() const;
	float GetTriggerValueR() const;
	vec2 GetStickL() const { return vec2(GetStickLX(), GetStickLY()); }
	vec2 GetStickR() const { return vec2(GetStickRX(), GetStickRY()); }

	// Gamepad Buttons:
	// TODO

	void ResetTime();

	void HandleEvent(const SDL_Event& ev);
	void Update();

private:
};
