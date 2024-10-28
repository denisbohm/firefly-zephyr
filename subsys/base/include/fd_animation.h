#ifndef fd_animation_h
#define fd_animation_h

#include "fd_source.h"

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stdint.h>

fd_source_push()

typedef struct {
  float duration;
  void *context;
  void (*start)(void *context);
  void (*waypoint)(void *context, float t);
  void (*stop)(void *context);
} fd_animation_step_t;

typedef enum {
  fd_animation_type_once,
  fd_animation_type_repeating,
} fd_animation_type_t;

typedef struct {
  const fd_animation_step_t *steps;
  uint32_t step_count;
  void *context;
  void (*finalize)(void *context);
  fd_animation_type_t type;
} fd_animation_sequence_t;

typedef struct {
  const fd_animation_sequence_t *sequence;
  uint32_t step;
  float elapsed;
  bool stop;
  bool cancel;
  const fd_animation_sequence_t *next;
} fd_animation_player_t;

#define fd_animation_sequence_declare(name, sequence_type, sequence_finalize,  \
                                      ...)                                     \
  const fd_animation_step_t name##_steps[] = {__VA_ARGS__};                    \
  const fd_animation_sequence_t name = {.steps = name##_steps,                 \
                                        .step_count = sizeof(name##_steps) /   \
                                                      sizeof(name##_steps[0]), \
                                        .finalize = sequence_finalize,         \
                                        .type = fd_animation_type_once}

#define fd_animation_once_declare(name, sequence_finalize, ...)                \
  fd_animation_sequence_declare(name, fd_animation_type_once,                  \
                                sequence_finalize, __VA_ARGS__)
#define fd_animation_repeating_declare(name, sequence_finalize, ...)           \
  fd_animation_sequence_declare(name, fd_animation_type_repeating,             \
                                sequence_finalize, __VA_ARGS__)

#define fd_animation_step_declare(...) ((fd_animation_step_t){__VA_ARGS__})

typedef struct {
  struct k_work_q *work_queue;
} fd_animation_configuration_t;

void fd_animation_initialize(const fd_animation_configuration_t *configuration);

void fd_animation_player_initialize(fd_animation_player_t *player);

void fd_animation_start(fd_animation_player_t *player,
                        const fd_animation_sequence_t *animation);
void fd_animation_stop(fd_animation_player_t *player);
void fd_animation_cancel(fd_animation_player_t *player);

float fd_animation_bezier_ease_in(float t);
float fd_animation_bezier_ease_out(float t);
float fd_animation_bezier_ease_in_out(float t);

fd_source_pop()

#endif
