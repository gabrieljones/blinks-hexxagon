#include "game_message.h"

#include <string.h>

#include "blink_state.h"
#include "game_map.h"
#include "game_player.h"
#include "game_state.h"
#include "game_state_play.h"
#include "src/blinks-broadcast/manager.h"
#include "src/blinks-broadcast/message.h"
#include "src/blinks-orientation/orientation.h"
#include "src/blinks-position/position.h"

#define MESSAGE_STATE_SEND_MESSAGE 0
#define MESSAGE_STATE_WAIT_FOR_RESULT 1

namespace game {

namespace message {

static void rcv_message_handler(byte message_id, byte src_face, byte* payload,
                                bool loop) {
  (void)src_face;

  if (loop) {
    // Ignore loops.
    return;
  }

  switch (message_id) {
    case MESSAGE_GAME_STATE_CHANGE:
      game::state::Data data;
      data.as_byte = payload[0];

      game::state::Set(data.state, true);
      game::state::SetSpecific(data.specific_state, true);
      game::state::SetPlayer(data.next_player +
                             1);  // TODO(bga): This limits us to 4 players.
      break;
    case MESSAGE_SELECT_ORIGIN:
      game::map::SetMoveOrigin(*((position::Coordinates*)payload));
      if ((int8_t)payload[0] == position::Local().x &&
          (int8_t)payload[1] == position::Local().y) {
        blink::state::SetOrigin(true);
        game::state::SetSpecific(GAME_STATE_PLAY_SELECT_TARGET, false);
        render::animation::ResetTimer();
      }
      break;
    case MESSAGE_SELECT_TARGET:
      game::map::SetMoveTarget(*((position::Coordinates*)payload));
      if ((int8_t)payload[0] == position::Local().x &&
          (int8_t)payload[1] == position::Local().y) {
        blink::state::SetTarget(true);
        game::state::SetSpecific(GAME_STATE_PLAY_MOVE_CONFIRMED, false);
      }
      break;
    case MESSAGE_FLASH:
      blink::state::StartColorOverride();
      break;
  }
}

static byte fwd_message_handler(byte message_id, byte src_face, byte dst_face,
                                byte* payload) {
  (void)src_face;
  (void)payload;

  byte len = 2;

  switch (message_id) {
    case MESSAGE_GAME_STATE_CHANGE:
      len = 1;
      break;
    case MESSAGE_EXTERNAL_PROPAGATE_COORDINATES:
      // Orientation and position where set when the message was received. See
      // consume() in game_map.cpp.
      payload[0] = orientation::RelativeLocalFace(dst_face);

      len = 4;
      break;
  }

  return len;
}

static bool sendMessage(byte message_id, const byte* payload,
                        byte payload_size) {
  broadcast::Message message;
  message.header.id = message_id;

  if (payload != nullptr) {
    memcpy(message.payload, payload, payload_size);
  }

  return broadcast::manager::Send(&message);
}

void Setup() {
  broadcast::manager::Setup(rcv_message_handler, fwd_message_handler);
}

bool SendGameStateChange() {
  byte payload = game::state::GetData();
  return sendMessage(MESSAGE_GAME_STATE_CHANGE, &payload, 1);
}

bool SendSelectOrigin(int8_t x, int8_t y) {
  byte payload[2] = {(byte)x, (byte)y};
  return sendMessage(MESSAGE_SELECT_ORIGIN, payload, 2);
}

bool SendSelectTarget(int8_t x, int8_t y) {
  byte payload[2] = {(byte)x, (byte)y};
  return sendMessage(MESSAGE_SELECT_TARGET, payload, 2);
}

bool SendExternalPropagateCoordinates(int8_t x, int8_t y, byte player) {
  byte payload[4] = {FACE_COUNT, (byte)x, (byte)y, player};
  return sendMessage(MESSAGE_EXTERNAL_PROPAGATE_COORDINATES, payload, 4);
}

bool SendFlash() {
  if (sendMessage(MESSAGE_FLASH, nullptr, 0)) {
    blink::state::StartColorOverride();

    return true;
  }

  return false;
}

}  // namespace message

}  // namespace game
