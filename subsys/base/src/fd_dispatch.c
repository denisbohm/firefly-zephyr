#include "fd_dispatch.h"

#include "fd_assert.h"

#include <string.h>

#ifndef fd_dispatch_item_limit
#define fd_dispatch_item_limit 10
#endif

typedef struct {
    fd_dispatch_filter_t filter;
    fd_dispatch_process_t process;
} fd_dispatch_item_t;

typedef struct {
    fd_dispatch_respond_t respond;
    fd_dispatch_item_t items[fd_dispatch_item_limit];
    uint32_t item_count;
} fd_dispatch_t;

fd_dispatch_t fd_dispatch;

fd_dispatch_item_t *fd_dispatch_get_item(fd_envelope_t *envelope) {
    for (uint32_t i = fd_dispatch.item_count; i > 0; --i) {
        fd_dispatch_item_t *item = &fd_dispatch.items[i - 1];
        if (item->filter(envelope)) {
            return item;
        }
    }
    return 0;
}

bool fd_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope) {
    fd_dispatch_item_t *item = fd_dispatch_get_item(envelope);
    if (item == 0) {
        return false;
    }
    return item->process(message, envelope, fd_dispatch.respond);
}

void fd_dispatch_add_process(fd_dispatch_process_t process, fd_dispatch_filter_t filter) {
    fd_assert(fd_dispatch.item_count < fd_dispatch_item_limit);
    if (fd_dispatch.item_count >= fd_dispatch_item_limit) {
        return;
    }
    fd_dispatch_item_t *item = &fd_dispatch.items[fd_dispatch.item_count++];
    item->process = process;
    item->filter = filter;
}

void fd_dispatch_initialize(fd_dispatch_respond_t respond) {
    memset(&fd_dispatch, 0, sizeof(fd_dispatch));
    fd_dispatch.respond = respond;
}
