#include "fd_test.h"

#include <string.h>

#ifdef CONFIG_FIREFLY_SUBSYS_BASE_TEST

typedef struct {
    const char *module;
    int identifier;
    bool (*function)(void *context);
    void *context;

    uint32_t count;
    uint32_t iteration;
} fd_test_item_t;

#ifndef fd_test_item_limit
#define fd_test_item_limit 4
#endif

typedef struct {
    fd_test_item_t items[fd_test_item_limit];
    uint32_t count;
} fd_test_t;

fd_test_t fd_test;

void fd_test_initialize(void) {
    memset(&fd_test, 0, sizeof(fd_test));
}

fd_test_item_t *fd_test_get_item(const char *module, int identifier) {
    for (uint32_t i = 0; i < fd_test.count; ++i) {
        fd_test_item_t *item = &fd_test.items[i];
        if ((item->identifier == identifier) && (strcmp(item->module, module) == 0)) {
            return item;
        }
    }
    return 0;
}

bool fd_test_should_fault(const char *module, int identifier) {
    fd_test_item_t *item = fd_test_get_item(module, identifier);
    if (item == 0) {
        return false;
    }
    return item->function(item->context);
}

void fd_test_add(const char *module, int identifier, bool (*function)(void *context), void *context) {
    if (fd_test.count >= fd_test_item_limit) {
        return;
    }
    fd_test_item_t *item = &fd_test.items[fd_test.count++];
    item->module = module;
    item->identifier = identifier;
    item->function = function;
    item->context = context;
}

bool fd_test_on_count(void *context) {
    fd_test_item_t *item = (fd_test_item_t *)context;
    ++item->iteration;
    if (item->iteration == item->count) {
        return true;
    }
    return false;
}

void fd_test_add_on_count(const char *module, int identifier, uint32_t count) {
    if (fd_test.count >= fd_test_item_limit) {
        return;
    }
    fd_test_item_t *item = &fd_test.items[fd_test.count++];
    item->module = module;
    item->identifier = identifier;
    item->function = fd_test_on_count;
    item->context = item;
    item->count = count;
}

#endif
