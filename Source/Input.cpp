// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Display.h"
#include "Input.h"

Input::Input(Display* aDisplay) : pDisplay(aDisplay) {

	pGamepad = SDL_GameControllerOpen(0);

	ticks = SDL_GetTicks();
	time = ticks * 0.001f;
}

void Input::SetTimeDilation(float multi) {
	dilation = multi > 0.f ? multi : 0.f;
}

static float PreprocessAxis(SDL_GameController* ctrl, SDL_GameControllerAxis axis) {
	if (ctrl) {
		let s = SDL_GameControllerGetAxis(ctrl, axis);
		let d = (double)s / 32767.0;
		return
			d < -1.0 ? -1.f :
			d > 1.0 ? 1.f :
			(d > -0.25 && d < 0.25) ? 0.f :
			d > 0.0 ? float((d - 0.25) / 0.75) :
			float((d + 0.25) / 0.75);
	}

	return 0.f;
};

float Input::GetStickLX() const {
	let keys = SDL_GetKeyboardState(nullptr);
	let kbx = keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A];
	return kbx + PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_LEFTX);
}

float Input::GetStickLY() const {
	let keys = SDL_GetKeyboardState(nullptr);
	let kby = keys[SDL_SCANCODE_W] - keys[SDL_SCANCODE_S];
	return kby - PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_LEFTY);
}

float Input::GetStickRX() const {
	let delta = IsMousePressed() ? 0.001f * mouseMovement.x : 0.f;
	return delta + PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_RIGHTX);
}

float Input::GetStickRY() const {
	let delta = IsMousePressed() ? -0.001f * mouseMovement.y : 0.f;
	return delta + PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_RIGHTY);
}

float Input::GetTriggerValueL() const {
	let keys = SDL_GetKeyboardState(nullptr);
	return keys[SDL_SCANCODE_Q] + PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
}

float Input::GetTriggerValueR() const {
	let keys = SDL_GetKeyboardState(nullptr);
	return keys[SDL_SCANCODE_E] + PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
}

bool Input::IsMousePressed() const {
	let mask = SDL_GetMouseState(nullptr, nullptr);
	return (mask & SDL_BUTTON_LEFT) != 0;
}

ivec2 Input::GetMousePosition() const {
	ivec2 result;
	SDL_GetMouseState(&result.x, &result.y);
	return result;
}

ivec2 Input::GetMouseMovement() const {
	ivec2 result;
	SDL_GetRelativeMouseState(&result.x, &result.y);
	return result;
}

void Input::HandleEvent(const SDL_Event& ev) {
}

void Input::ResetTime() {
	ticks = SDL_GetTicks();
	time = ticks * 0.001f;
	deltaTicks = 0;
	deltaTime = 0.f;
}

void Input::Update() {
	let nextTicks = SDL_GetTicks();
	let nextTime = nextTicks * (0.001f * dilation);

	deltaTicks = nextTicks - ticks;
	ticks = nextTicks;

	deltaTime = nextTime - time;
	time = nextTime;

	let mousePosition = GetMousePosition();
	mouseMovement = vec2(mousePosition - prevMousePosition) / deltaTime;
	prevMousePosition = mousePosition;
	if (IsMousePressed()) {
		let center = ivec2(0.5f * vec2(pDisplay->GetScreenSize()));
		SDL_WarpMouseInWindow(pDisplay->GetWindow(), center.x, center.y);	
		prevMousePosition = center;
	}
}

