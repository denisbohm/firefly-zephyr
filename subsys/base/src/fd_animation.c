#include "fd_animation.h"

#include "fd_assert.h"

#include <zephyr/kernel.h>

#include <string.h>

fd_source_push()

typedef struct {
    const fd_animation_configuration_t *configuration;
    struct k_timer timer;
    struct k_work work;
    fd_animation_player_t *players[CONFIG_FIREFLY_SUBSYS_BASE_ANIMATION_PLAYER_LIMIT];
    uint32_t player_count;
    bool active;
} fd_animation_t;

fd_animation_t fd_animation;

void fd_animation_player_tick(fd_animation_player_t *player) {
    const fd_animation_sequence_t *sequence = player->sequence;
    if (sequence == 0) {
        return;
    }

    player->elapsed += 0.025f;
    const fd_animation_step_t *step = &sequence->steps[player->step];
    if ((player->elapsed < step->duration) && !player->cancel) {
        float t = player->elapsed / step->duration;
        if (step->waypoint) {
            step->waypoint(step->context, t);
        }
        return;
    }

    if (step->stop) {
        step->stop(step->context);
    }

    player->elapsed = 0.0f;
    ++player->step;
    if (player->step >= sequence->step_count) {
        player->step = 0;
        if ((player->sequence->type == fd_animation_type_once) || player->stop) {
            player->sequence = 0;
            player->stop = false;
            player->cancel = false;
            if (sequence->finalize) {
                sequence->finalize(sequence->context);
            }
            if (player->next == 0) {
                return;
            }
            sequence = player->next;
            player->next = 0;
            player->sequence = sequence;
        }
    }

    step = &sequence->steps[player->step];
    if (step->start) {
        step->start(step->context);
    }
}

void fd_animation_work(struct k_work *work) {
    bool any_active = false;
    for (uint32_t i = 0; i < fd_animation.player_count; ++i) {
        fd_animation_player_t *player = fd_animation.players[i];
        fd_animation_player_tick(player);
    }
    for (uint32_t i = 0; i < fd_animation.player_count; ++i) {
         fd_animation_player_t *player = fd_animation.players[i];
         if (player->sequence != 0) {
             any_active = true;
         }
     }
     if (!any_active) {
         k_timer_stop(&fd_animation.timer);
         fd_animation.active = false;
     }
}

void fd_animation_timer_irq(struct k_timer *timer) {
     k_work_submit_to_queue(fd_animation.configuration->work_queue, &fd_animation.work);
}

void fd_animation_initialize(const fd_animation_configuration_t *configuration) {
    memset(&fd_animation, 0, sizeof(fd_animation));
    fd_animation.configuration = configuration;

    k_work_init(&fd_animation.work, fd_animation_work);
    k_timer_init(&fd_animation.timer, fd_animation_timer_irq, NULL);
}

void fd_animation_player_initialize(fd_animation_player_t *player) {
    fd_assert(fd_animation.player_count < ARRAY_SIZE(fd_animation.players));

    memset(player, 0, sizeof(*player));
    fd_animation.players[fd_animation.player_count++] = player;
}

void fd_animation_start(fd_animation_player_t *player, const fd_animation_sequence_t *sequence) {
    if (player->sequence) {
        player->stop = true;
        player->next = sequence;
    } else {
        player->sequence = sequence;
        player->step = 0;
        player->elapsed = 0.0f;
        player->stop = false;
        player->cancel = false;
        const fd_animation_step_t *step = &sequence->steps[player->step];
        if (step->start) {
            step->start(step->context);
        }
    }
    if (!fd_animation.active) {
        fd_animation.active = true;
        k_timer_start(&fd_animation.timer, K_MSEC(25), K_MSEC(25));
    }
}

void fd_animation_stop(fd_animation_player_t *player) {
    player->stop = true;
}

void fd_animation_cancel(fd_animation_player_t *player) {
    player->next = 0;
    player->stop = true;
    player->cancel = true;
}

float fd_animation_bezier_ease_in(float t) {
    return t * t * (3.0f - 2.0f * t);
}

float fd_animation_bezier_ease_out(float t) {
    return 1.0f - fd_animation_bezier_ease_in(t);
}

float fd_animation_bezier_ease_in_out(float t) {
    if (t <= 0.5f) {
        return fd_animation_bezier_ease_in(t * 2.0f);
    } else {
        return fd_animation_bezier_ease_out((t - 0.5f) * 2.0f);
    }
}

fd_source_pop()
