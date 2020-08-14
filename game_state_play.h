#ifndef GAME_STATE_PLAY_H_
#define GAME_STATE_PLAY_H_

#include <blinklib.h>

// Play specific states.
#define GAME_STATE_PLAY_SELECT_ORIGIN 0
#define GAME_STATE_PLAY_ORIGIN_SELECTED 1
#define GAME_STATE_PLAY_SELECT_TARGET 2
#define GAME_STATE_PLAY_TARGET_SELECTED 3
#define GAME_STATE_PLAY_CONFIRM_MOVE 4
#define GAME_STATE_PLAY_MOVE_CONFIRMED 5

#define GAME_STATE_PLAY_PASS_TURN 6

namespace game {

namespace state {

namespace play {

void Handler(bool state_changed, byte* state, byte* specific_state);

void HandleReceiveMessage(byte message_id, byte* payload);
byte HandleForwardMessage(byte message_id, byte src_face, byte dst_face,
                          byte* payload);
void HandleReceiveReply(byte message_id, const byte* payload);
byte HandleForwardReply(byte message_id, byte* payload);

}  // namespace play

}  // namespace state

}  // namespace game

#endif