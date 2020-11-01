#include "blink_state.h"

#include <blinklib.h>

#include "game_state.h"
#include "game_state_end_render.h"
#include "game_state_idle_render.h"
#include "game_state_play_render.h"
#include "game_state_setup_render.h"

namespace blink {

namespace state {

struct BlinkState {
  bool origin;
  bool target;
  bool animating;
  bool self_destruct;
  bool locked;
  byte player;
  byte target_type;
};
static BlinkState state_;

static Timer color_override_timer_;

static byte animating_param_;
static bool (*animating_function_)(byte param);

void SetOrigin(bool origin) { state_.origin = origin; }
bool __attribute__((noinline)) GetOrigin() { return state_.origin; }

void SetTarget(bool target) { state_.target = target; }
bool __attribute__((noinline)) GetTarget() { return state_.target; }

void SetTargetType(byte target_type) { state_.target_type = target_type; }
byte GetTargetType() { return state_.target_type; }

void __attribute__((noinline)) SetPlayer(byte player) {
  state_.player = player;
}
byte GetPlayer() { return state_.player; }

void SetSelfDestruct(bool self_destruct) {
  state_.self_destruct = self_destruct;
}
byte GetSelfDestruct() { return state_.self_destruct; }

void SetAnimating(bool animating) { state_.animating = animating; }
bool GetAnimating() { return state_.animating; }

void SetAnimatingParam(byte animating_param) {
  animating_param_ = animating_param;
}
void SetAnimatingFunction(bool (*animating_function)(byte param)) {
  animating_function_ = animating_function;
}

bool RunAnimatingFunction() { return animating_function_(animating_param_); }

void __attribute__((noinline)) StartColorOverride() {
  color_override_timer_.set(200);
}

bool GetColorOverride() { return !color_override_timer_.isExpired(); }

void SetLocked(bool locked) { state_.locked = locked; }
bool GetLocked() { return state_.locked; }

void __attribute__((noinline)) Reset() {
  state_.origin = false;
  state_.target = false;
  state_.animating = false;
  state_.self_destruct = false;
  state_.locked = false;
  state_.target_type = BLINK_STATE_TARGET_TYPE_NONE;
  state_.player = 0;
}

void __attribute__((noinline)) Render(byte game_state) {
  // "Render" our face value.
  FaceValue face_value;

  face_value.self_destruct = GetSelfDestruct();
  face_value.target = GetTarget();
  face_value.player = GetPlayer();

  setValueSentOnAllFaces(face_value.as_byte);

  if (GetColorOverride()) {
    setColor(WHITE);

    return;
  }

  // Now render the state specific colors/animations.
  switch (game_state) {
    case GAME_STATE_IDLE:
      game::state::idle::Render();
      break;
    case GAME_STATE_SETUP:
      game::state::setup::Render();
      break;
    case GAME_STATE_PLAY:
      game::state::play::Render();
      break;
    case GAME_STATE_END:
      game::state::end::Render();
      break;
  }
}

}  // namespace state

}  // namespace blink
