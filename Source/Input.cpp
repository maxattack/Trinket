#include "Input.h"

Input::Input() {

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
	return PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_LEFTX);
}

float Input::GetStickLY() const {
	return -PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_LEFTY);
}

float Input::GetStickRX() const {
	return PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_RIGHTX);
}

float Input::GetStickRY() const {
	return -PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_RIGHTY);
}

float Input::GetTriggerValueL() const {
	return PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
}

float Input::GetTriggerValueR() const {
	return PreprocessAxis(pGamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
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
}

