#include "game_state_setup_render.h"

#include "blink_state.h"
#include "game_player.h"
#include "game_state.h"
#include "game_state_setup.h"
#include "render_config.h"

namespace game {

namespace state {

namespace setup {

void Render() {
  if (game::state::GetSpecific() != GAME_STATE_SETUP_MAP) {
    Color player_color = game::player::GetColor(blink::state::GetPlayer());

    setColor(dim(player_color, RENDER_CONFIG_SETUP_STATE_COLOR_DIM));
  }
}

}  // namespace setup

}  // namespace state

}  // namespace game