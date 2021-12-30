#include "fd_event.h"

#include "fd_assert.h"

#include <string.h>

typedef struct {
    const char *name;
    uint32_t identifier;
    volatile bool is_set;
} fd_event_item_t;

typedef struct {
    uint32_t identifier;
    fd_event_callback_t callback;
} fd_event_consumer_t;

#ifndef fd_event_item_limit
#define fd_event_item_limit 32
#endif

#ifndef fd_event_consumer_limit
#define fd_event_consumer_limit 32
#endif

typedef struct {
    fd_event_item_t items[fd_event_item_limit];
    uint32_t item_count;
    
    fd_event_consumer_t consumers[fd_event_consumer_limit];
    uint32_t consumer_count;
} fd_event_t;

fd_event_t fd_event;

void fd_event_initialize(void) {
    memset(&fd_event, 0, sizeof(fd_event));
}

uint32_t fd_event_get_identifier(const char *name) {
    for (uint32_t i = 0; i < fd_event.item_count; ++i) {
        fd_event_item_t *item = &fd_event.items[i];
        if (strcmp(name, item->name) == 0) {
            return item->identifier;
        }
    }
    fd_assert(fd_event.item_count < fd_event_item_limit);
    fd_event_item_t *item = &fd_event.items[fd_event.item_count];
    item->name = name;
    item->identifier = fd_event.item_count;
    ++fd_event.item_count;
    return item->identifier;
}

void fd_event_add_callback(uint32_t identifier, fd_event_callback_t callback) {
    fd_assert(identifier < fd_event.item_count);
    fd_assert(fd_event.consumer_count < fd_event_consumer_limit);
    fd_event_consumer_t *consumer = &fd_event.consumers[fd_event.consumer_count];
    consumer->identifier = identifier;
    consumer->callback = callback;
    ++fd_event.consumer_count;
}

void fd_event_set(uint32_t identifier) {
    fd_assert(identifier < fd_event.item_count);
    fd_event_item_t *item = &fd_event.items[identifier];
    item->is_set = true;
}

void fd_event_set_from_interrupt(uint32_t identifier) {
    fd_event_set(identifier);
}

void fd_event_process(void) {
    bool is_set[fd_event_item_limit];
    for (uint32_t i = 0; i < fd_event.item_count; ++i) {
        fd_event_item_t *item = &fd_event.items[i];
        is_set[i] = item->is_set;
        if (is_set[i]) {
            // disable interrupts
            item->is_set = false;
            // enable interrupts
        }
    }
    for (uint32_t i = 0; i < fd_event.consumer_count; ++i) {
        fd_event_consumer_t *consumer = &fd_event.consumers[i];
        uint32_t identifier = consumer->identifier;
        if (is_set[identifier]) {
            consumer->callback(identifier);
        }
    }
}
