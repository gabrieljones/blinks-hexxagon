#include "game_state_play_render.h"

#include "blink_state.h"
#include "game_map.h"
#include "game_player.h"
#include "game_state.h"
#include "game_state_play.h"
#include "render_animation.h"
#include "render_config.h"

namespace game {

namespace state {

namespace play {

void Render() {
  // TODO(bga): Attempts to reduce code size here failed miserably. But my
  // intuition is that it can be done. Try again?

  byte player = blink::state::GetPlayer();
  Color player_color = game::player::GetColor(player);

  byte game_player = game::state::GetPlayer();
  byte specific_state = game::state::GetSpecific();

  // Set initial color for this Blink. This is required as some of the render
  // animations only render an overlay.
  setColor(player_color);

  if ((blink::state::GetOrigin() &&
       (specific_state < GAME_STATE_PLAY_TARGET_SELECTED)) ||
      (blink::state::GetTarget() &&
       (specific_state == GAME_STATE_PLAY_MOVE_CONFIRMED))) {
    // We are the origin and and a target was not selected yet or we are a
    // target and the move is being confirmed. Render spinning animation.
    render::animation::Spinner(RENDER_CONFIG_PLAY_STATE_SPINNER_COLOR,
                               RENDER_CONFIG_PLAY_STATE_SPINNER_SLOWDOWN);
    return;
  }

  if (player == game_player) {
    // We are the current player.
    if ((specific_state < GAME_STATE_PLAY_MOVE_CONFIRMED) &&
        game::map::GetStatistics().local_blink_empty_space_in_range) {
      // Move has not been confirmed yet and the Blink has a space in range to
      // move to (or is a possible target). Render pulse animation.
      render::animation::Pulse(player_color,
                               RENDER_CONFIG_PLAY_STATE_PULSE_START_DIM,
                               RENDER_CONFIG_PLAY_STATE_PULSE_SLOWDOWN);
      return;
    }
  } else if (player != 0) {
    // We are an enemy player Blink.
    if ((specific_state == GAME_STATE_PLAY_MOVE_CONFIRMED) &&
        position::Distance(game::map::GetMoveTarget()) == 1) {
      // Move is being confirmed and we are neighbors of the target. Render
      // explosion animation.
      if (render::animation::Explosion(player_color)) {
        blink::state::SetPlayer(game_player);
      }

      return;
    }
  }

  // We did not match anything above, which means we are a locked player Blink,
  // an unaffected enemy Blink or a non-target empty Blink.
  if (blink::state::GetTargetType() == BLINK_STATE_TARGET_TYPE_NONE) {
    setColor(dim(player_color, RENDER_CONFIG_PLAY_STATE_COLOR_DIM));
  }
}

}  // namespace play

}  // namespace state

}  // namespace game
