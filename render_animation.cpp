#include "render_animation.h"

// (255 * 3) + 200
#define RENDER_ANIMATION_EXPLOSION_MS 965

#ifndef RENDER_ANIMATION_TAKE_OVER_DISABLE_LIGHTNING
#define RENDER_ANIMATION_LIGHTNING_MS 100
#endif

namespace render {

namespace animation {

static Timer timer_;

static bool reverse_ = true;
static bool animation_started_ = false;

#ifndef RENDER_ANIMATION_TAKE_OVER_DISABLE_LIGHTNING
static byte end_;

static bool take_over_lightning_done_;
#endif

static bool reset_timer_if_expired(word ms) {
  if (timer_.isExpired()) {
    if (!animation_started_) timer_.set(ms);

    return true;
  }

  return false;
}

static bool explosion(const Color& base_color) {
  if (reset_timer_if_expired(RENDER_ANIMATION_EXPLOSION_MS)) {
    if (animation_started_) {
      animation_started_ = false;

      return true;
    } else {
      animation_started_ = true;
    }
  }

  if (timer_.getRemaining() > 200) {
    setColor(lighten(base_color, 255 - ((timer_.getRemaining() - 200) / 3)));
  } else if (timer_.getRemaining() > 100) {
    setColor(WHITE);
  } else {
    setColor(OFF);
  }

  return false;
}

#ifndef RENDER_ANIMATION_TAKE_OVER_DISABLE_LIGHTNING
static bool lightning(byte origin_face) {
  if (reset_timer_if_expired(RENDER_ANIMATION_LIGHTNING_MS)) {
    if (animation_started_) {
      animation_started_ = false;

      return true;
    } else {
      animation_started_ = true;
    }

    end_ = millis() % 3;  // Pseudo pseudo-random number. :)
  }

  setColorOnFace(WHITE, origin_face);

  if (timer_.getRemaining() < RENDER_ANIMATION_LIGHTNING_MS - 25) {
    byte destination_face = (origin_face + 2 + end_) % FACE_COUNT;

    setColorOnFace(WHITE, destination_face);
  }

  return false;
}
#endif

void ResetPulseTimer() {
  timer_.set(0);
  reverse_ = true;
}

void Pulse(const Color& base_color, byte start, byte slowdown) {
  if (reset_timer_if_expired((255 - start) * slowdown)) {
    reverse_ = !reverse_;
  }

  byte base_brightness = timer_.getRemaining() / slowdown;

  byte brightness = reverse_ ? 255 - base_brightness : start + base_brightness;

  setColor(dim(base_color, brightness));
}

void Spinner(const Color& spinner_color, byte num_faces, byte slowdown) {
  reset_timer_if_expired((FACE_COUNT * slowdown) - 1);

  byte f = (FACE_COUNT - 1) - timer_.getRemaining() / slowdown;

  byte step = FACE_COUNT / num_faces;

  for (byte face = 0; face < FACE_COUNT; face += step) {
    setColorOnFace(spinner_color, (f + face) % FACE_COUNT);
  }
}

#ifndef RENDER_ANIMATION_TAKE_OVER_DISABLE_LIGHTNING
bool TakeOver(const Color& base_color, byte origin_face) {
  if (!take_over_lightning_done_ && !lightning(origin_face)) return false;

  take_over_lightning_done_ = true;

  if (!explosion(base_color)) return false;

  take_over_lightning_done_ = false;

  return true;
}
#else
bool TakeOver(const Color& base_color) { return explosion(base_color); }
#endif

}  // namespace animation

}  // namespace render
