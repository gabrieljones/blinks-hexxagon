
#include "game_state.h"

#include "debug.h"
#include "game_message.h"
#include "game_player.h"

namespace game {

namespace state {

struct State {
  byte current : 2;
  byte previous : 2;
  byte player : 3;
  bool from_network : 1;
};
static State state_;

struct SpecificState {
  byte current : 4;
  byte previous : 4;
};
static SpecificState specific_state_;

static BlinkCount blink_count_;

void Set(byte state, bool from_network) {
  state_.previous = state_.current;
  state_.current = state;

  state_.from_network = from_network;
}

byte Get() { return state_.current; }

void SetSpecific(byte specific_state, bool from_network) {
  specific_state_.previous = specific_state_.current;
  specific_state_.current = specific_state;

  state_.from_network = from_network;
}

byte GetSpecific() { return specific_state_.current; }

void SetPlayer(byte player) { state_.player = player; }

byte __attribute__((noinline)) GetPlayer() { return state_.player; }

void NextPlayer() {
  byte current_player = GetPlayer();

  byte next_player = game::player::GetNext(current_player);
  while ((blink_count_[next_player] == 0 || next_player == 0) &&
         next_player != current_player) {
    next_player = game::player::GetNext(next_player);
  }

  SetPlayer(next_player);
}

byte UpdateBoardState() {
  broadcast::Message reply;
  if (!game::message::SendCheckBoard(&reply)) {
    return GAME_STATE_UPDATE_BOARD_STATE_UPDATING;
  }

  SetBlinkCount(reply.payload);

  if (reply.payload[0] == 0) {
    // We need at least one empty Blink.
    return GAME_STATE_UPDATE_BOARD_STATE_ERROR;
  }

  byte players_count = 0;
  for (byte i = 1; i < GAME_PLAYER_MAX_PLAYERS + 1; ++i) {
    if (reply.payload[i] > 0) players_count++;
  }

  if (players_count < 2) {
    // We need at least two players.
    return GAME_STATE_UPDATE_BOARD_STATE_ERROR;
  }

  return GAME_STATE_UPDATE_BOARD_STATE_OK;
}

void __attribute__((noinline)) SetBlinkCount(BlinkCount blink_count) {
  for (byte i = 0; i < GAME_PLAYER_MAX_PLAYERS + 1; ++i) {
    blink_count_[i] = blink_count == nullptr ? 0 : blink_count[i];
  }
}

byte GetBlinkCount(byte player) { return blink_count_[player]; }

void Reset() {
  state_.current = GAME_STATE_IDLE;
  state_.previous = GAME_STATE_IDLE;
  state_.player = 0;
  state_.from_network = false;
  specific_state_.current = 0;
  specific_state_.previous = 0;
  SetBlinkCount(nullptr);
}

bool Changed(bool include_specific) {
  return include_specific
             ? state_.current != state_.previous ||
                   specific_state_.current != specific_state_.previous
             : state_.current != state_.previous;
}

bool Propagate() {
  if (!Changed() || state_.from_network) return true;

  game::message::GameStateChangeData data;

  data.state = state_.current;
  data.specific_state = specific_state_.current;
  data.next_player = state_.player - 1;

  if (!game::message::SendGameStateChange(data.value)) {
    return false;
  }

  return true;
}

}  // namespace state

}  // namespace game
