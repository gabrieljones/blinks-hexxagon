#include "game_map.h"

#include <string.h>

#include "blink_state.h"
#include "game_message.h"
#include "game_state.h"
#include "src/blinks-broadcast/handler.h"
#include "src/blinks-broadcast/manager.h"
#include "src/blinks-orientation/orientation.h"

// As we can not known beforehand how many Blinks are in the map, we need a
// timeout to consider that everything cleared up. This is the time since the
// last mapping message was received.
#define GAME_MAP_PROPAGATION_TIMEOUT 3000

namespace game {

namespace map {

struct MapData {
  int16_t x : 6;
  int16_t y : 6;
  uint16_t player : 4;
};

static MapData map_[GAME_MAP_MAX_BLINKS];
static byte map_index_;
static byte map_propagation_index_;

Stats stats_;

static Timer propagation_timer_;

static bool move_commited_;

struct MoveData {
  position::Coordinates origin;
  position::Coordinates target;
};

static MoveData move_data_;

static void maybe_propagate() {
  if (map_index_ != map_propagation_index_) {
    if (game::message::SendExternalPropagateCoordinates(
            (int8_t)map_[map_propagation_index_].x,
            (int8_t)map_[map_propagation_index_].y,
            (byte)map_[map_propagation_index_].player)) {
      map_propagation_index_++;
    }
  }
}

static MapData* find_entry_in_map(int8_t x, int8_t y) {
  for (byte i = 0; i < map_index_; ++i) {
    if (x == map_[i].x && y == map_[i].y) {
      return &map_[i];
    }
  }

  return nullptr;
}

static void __attribute__((noinline))
add_to_map(int8_t x, int8_t y, byte player) {
  map_[map_index_].x = x;
  map_[map_index_].y = y;
  map_[map_index_].player = player;
  map_index_++;
}

static void add_local_to_map() {
  add_to_map(position::Local().x, position::Local().y,
             blink::state::GetPlayer());
}

static void __attribute__((noinline))
update_blinks(position::Coordinates coordinates, byte player,
              bool update_neighbors) {
  for (byte i = 0; i < map_index_; ++i) {
    if ((map_[i].x == coordinates.x) && (map_[i].y == coordinates.y)) {
      // This is the position being updated. Change it to belong to the given
      // player.
      map_[i].player = player;
    }

    if (update_neighbors && (map_[i].player != 0) &&
        (position::coordinates::Distance(
             coordinates, {(int8_t)map_[i].x, (int8_t)map_[i].y}) == 1)) {
      // Neighbor from a different player. Now belongs to the given player.
      map_[i].player = player;
    }
  }
}

void consume(const broadcast::Message* message, byte local_absolute_face) {
  propagation_timer_.set(GAME_MAP_PROPAGATION_TIMEOUT);

  if (map_index_ == 0) {
    // We do not have anything on our map, so we need to initialize our
    // local data and add it to the map.
    orientation::Setup(message->payload[0], local_absolute_face);
    position::Setup(orientation::RelativeLocalFace(local_absolute_face),
                    (int8_t)message->payload[1], (int8_t)message->payload[2]);

    add_local_to_map();
  }

  if (find_entry_in_map((int8_t)message->payload[1],
                        (int8_t)message->payload[2]) != nullptr) {
    setColor(OFF);

    return;
  }

  setColor(game::player::GetColor(blink::state::GetPlayer()));

  add_to_map((int8_t)message->payload[1], (int8_t)message->payload[2],
             message->payload[3]);
}

void Setup() {
  broadcast::message::handler::Set(
      {MESSAGE_EXTERNAL_PROPAGATE_COORDINATES, consume});
}

void Process() { maybe_propagate(); }

void __attribute__((noinline)) StartMapping() {
  // We are the mapping origin. Add ourselves to the map.
  add_local_to_map();

  propagation_timer_.set(GAME_MAP_PROPAGATION_TIMEOUT);
}

bool GetMapping() { return (!propagation_timer_.isExpired()); }

void ComputeMapStats() {
  memset(&stats_, 0, sizeof(Stats));

  for (byte i = 0; i < map_index_; ++i) {
    const MapData& map_data = map_[i];
    // Update number of players.
    if (map_data.player != 0 &&
        stats_.player[map_data.player].blink_count == 0) {
      stats_.player_count++;
    }

    // Update player blink count.
    stats_.player[map_data.player].blink_count++;

    // Update player can move.
    for (byte j = 0; j < map_index_; ++j) {
      if ((map_[j].player == 0) &&
          (position::coordinates::Distance(
               {(int8_t)map_data.x, (int8_t)map_data.y},
               {(int8_t)map_[j].x, (int8_t)map_[j].y}) <= 2)) {
        stats_.player[map_data.player].can_move = true;
        break;
      }
    }

    // Update empty space in range.
    if (!(stats_.local_blink_empty_space_in_range)) {
      stats_.local_blink_empty_space_in_range =
          ((map_data.player == 0) &&
           (position::Distance(position::Coordinates{
                (int8_t)map_data.x, (int8_t)map_data.y}) <= 2));
    }
  }
}

static void set_data(int8_t x, int8_t y, position::Coordinates* coordinates,
                     void (*setter)(bool)) {
  coordinates->x = x;
  coordinates->y = y;

  if (x == position::Local().x && y == position::Local().y) setter(true);

  move_commited_ = false;
}

void SetMoveOrigin(int8_t x, int8_t y) {
  set_data(x, y, &move_data_.origin, blink::state::SetOrigin);
}

void SetMoveTarget(int8_t x, int8_t y) {
  set_data(x, y, &move_data_.target, blink::state::SetTarget);
}

void __attribute__((noinline)) CommitMove() {
  if (move_commited_) return;

  if (position::coordinates::Distance(move_data_.origin, move_data_.target) >
      1) {
    update_blinks(move_data_.origin, 0, false);
  }

  update_blinks(move_data_.target, game::state::GetPlayer(), true);

  ComputeMapStats();

  move_commited_ = true;
}

const Stats& GetStats() { return stats_; }

bool __attribute__((noinline)) ValidState() {
  return (stats_.player[0].blink_count > 0) && (stats_.player_count > 1);
}

void Reset() {
  map_index_ = 0;
  map_propagation_index_ = 0;

  ComputeMapStats();
}

}  // namespace map

}  // namespace game