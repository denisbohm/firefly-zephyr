#ifndef fd_key_value_store_h
#define fd_key_value_store_h

#include <pb_decode.h>
#include <pb_encode.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *context;
    void (*was_erased)(void *context);
    void (*was_set)(void *context, const char *key);
    void (*was_removed)(void *context, const char *key);
} fd_key_value_store_listener_t;

void fd_key_value_store_initialize(void);

void fd_key_value_store_add_listener(const fd_key_value_store_listener_t *listener);

void fd_key_value_store_erase(void);

bool fd_key_value_store_set(const char *key, const pb_msgdesc_t *descriptor, const void *object);
bool fd_key_value_store_get(const char *key, const pb_msgdesc_t *descriptor, void *object);
bool fd_key_value_store_remove(const char *key);

#endif