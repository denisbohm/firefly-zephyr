#include "fd_key_value_store.h"

#include <fd_assert.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

/*
Persistent storage for key (string) / value (protobuf message) pairs in MCU flash pages.

There is a FIFO circular buffer of pages.

Each page has:
 * fixed size metadata records that are allocated from the end of the page
 * variable size data (key & message) that is allocated from the start of the page
 * at least one empty metadata record to deliniate where the metadata records end

Garbage collection is done when a new KVP will not fit in the current page.
The live objects from the first used page are copied to the next free page,
then the first used page is erased.  This is repeated until the new KVP fits
into the current page.
*/

#define fd_key_value_store_area_id FIXED_PARTITION_ID(persist_partition)

typedef struct {
    uint16_t offset;
    uint16_t size;
} fd_key_value_store_blob_t;

#define fd_key_value_store_metadata_flags_removed 0x000001

typedef struct __attribute__((packed)) {
    uint32_t flags;
    fd_key_value_store_blob_t key;
    fd_key_value_store_blob_t value;
} fd_key_value_store_metadata_t;

typedef struct {
    uint8_t buffer[16];
    size_t count;
} fd_key_value_store_ostream_state_t;

typedef struct {
    const uint8_t *value;
    size_t size;
    uint32_t count;
    bool is_equal;
} fd_key_value_store_equal_stream_state_t;

typedef struct {
    uint32_t page;
    size_t data_byte_count;
    size_t metadata_count;
} fd_key_value_store_currrent_t;

typedef struct {
    uint32_t base_address;
    uint32_t page_count;
    size_t page_size;
    size_t minumum_write_size;
    bool is_full;
    fd_key_value_store_currrent_t current;
    uint32_t data_write_offset;
    fd_key_value_store_ostream_state_t ostream_state;
    uint32_t garbage_collection_count;
    fd_key_value_store_equal_stream_state_t equal_stream_state;
    fd_key_value_store_listener_t listeners[CONFIG_FIREFLY_SUBSYS_STORAGE_KEY_VALUE_STORE_LISTENER_LIMIT];
    uint32_t listener_count;
} fd_key_value_store_t;

fd_key_value_store_t fd_key_value_store;

void fd_key_value_store_add_listener(const fd_key_value_store_listener_t *listener) {
    fd_assert(fd_key_value_store.listener_count < ARRAY_SIZE(fd_key_value_store.listeners));
    if (fd_key_value_store.listener_count >= ARRAY_SIZE(fd_key_value_store.listeners)) {
        return;
    }
    fd_key_value_store.listeners[fd_key_value_store.listener_count++] = *listener;
}

uint32_t fd_key_value_store_get_page_address(uint32_t page) {
    return fd_key_value_store.base_address + page * fd_key_value_store.page_size;
}

size_t fd_key_value_store_get_write_size(size_t size) {
    size_t n = fd_key_value_store.minumum_write_size - 1;
    return (size + n) & ~n;
}

fd_key_value_store_metadata_t *fd_key_value_store_get_metadata(uint32_t page, int index) {
    uint32_t page_address = fd_key_value_store_get_page_address(page);
    uint32_t offset = fd_key_value_store.page_size - (index + 1) * sizeof(fd_key_value_store_metadata_t);
    return (fd_key_value_store_metadata_t *)(page_address + offset);
}

uint32_t fd_key_value_store_get_metadata_count(uint32_t page) {
    uint32_t max_count = fd_key_value_store.page_size / sizeof(fd_key_value_store_metadata_t);
    for (uint32_t i = 0; i < max_count; ++i) {
        fd_key_value_store_metadata_t *metadata = fd_key_value_store_get_metadata(page, i);
        if (metadata->flags == 0xffffffff) {
            return i;
        }
    }
    return 0;
}

bool fd_key_value_store_is_page_empty(uint32_t page) {
    // if there are any metadata records then the page is not empty
    fd_key_value_store_metadata_t *metadata = fd_key_value_store_get_metadata(page, 0);
    if (metadata->flags != 0xffffffff) {
        return false;
    }

    // check the page and assert that it is actually empty
    uint8_t *address = (uint8_t *)fd_key_value_store_get_page_address(page);
    bool empty = true;
    for (uint32_t i = 0; i < fd_key_value_store.page_size; ++i) {
        if (address[i] != 0xff) {
            empty = false;
        }
    }
    fd_assert(empty);
    return true;
}

bool fd_key_value_store_get_current_page(uint32_t *current_page) {
    fd_key_value_store.is_full = false;
    bool found_occupied = false;
    uint32_t first_occupied_page = 0;
    for (uint32_t i = 0; i < fd_key_value_store.page_count; ++i) {
        bool empty = fd_key_value_store_is_page_empty(i);
        if (!empty) {
            found_occupied = true;
            first_occupied_page = i;
            break;
        }
    }
    if (!found_occupied) {
        *current_page = 0;
        return true;
    }
    
    bool found_empty = false;
    uint32_t first_empty_page = 0;
    for (uint32_t i = 1; i < fd_key_value_store.page_count; ++i) {
        uint32_t page = (first_occupied_page + i) % fd_key_value_store.page_count;
        bool empty = fd_key_value_store_is_page_empty(page);
        if (empty) {
            found_empty = true;
            first_empty_page = page;
            break;
        }
    }
    if (!found_empty) {
        *current_page = 0;
        return false;
    }
    *current_page = (first_empty_page + fd_key_value_store.page_count - 1) % fd_key_value_store.page_count;
    return true;
}

void fd_key_value_store_update_current(void) {
    fd_key_value_store.current.data_byte_count = 0;
    fd_key_value_store.current.metadata_count = 0;
    fd_key_value_store.is_full = !fd_key_value_store_get_current_page(&fd_key_value_store.current.page);
    if (fd_key_value_store.is_full) {
        return;
    }

    fd_key_value_store.current.metadata_count = fd_key_value_store_get_metadata_count(fd_key_value_store.current.page);
    if (fd_key_value_store.current.metadata_count == 0) {
        return;
    }
    fd_key_value_store_metadata_t *metadata = fd_key_value_store_get_metadata(fd_key_value_store.current.page, fd_key_value_store.current.metadata_count - 1);
    size_t data_size = fd_key_value_store_get_write_size(metadata->key.size + metadata->value.size);
    fd_key_value_store.current.data_byte_count = metadata->key.offset + data_size;
}

void *fd_key_value_store_lookup_on_page(uint32_t page, const char *key, size_t key_size, size_t *value_size) {
    uint32_t page_address = fd_key_value_store_get_page_address(page);
    uint32_t metadata_count = fd_key_value_store_get_metadata_count(page);
    for (uint32_t i = 0; i < metadata_count; ++i) {
        fd_key_value_store_metadata_t *metadata = fd_key_value_store_get_metadata(page, metadata_count - 1 - i);
        char *key_address = (char *)(page_address + metadata->key.offset);
        if ((metadata->key.size == key_size) && (strncmp(key, key_address, key_size) == 0)) {
            if ((metadata->flags & fd_key_value_store_metadata_flags_removed) != 0) {
                break;
            }
            *value_size = metadata->value.size;
            return (void *)(page_address + metadata->value.offset);
        }
    }
    *value_size = 0;
    return NULL;
}

void *fd_key_value_store_lookup(const char *key, size_t key_size, size_t *value_size) {
    uint32_t page = fd_key_value_store.current.page;
    do {
        void *data = fd_key_value_store_lookup_on_page(page, key, key_size, value_size);
        if (data != NULL) {
            return data;
        }

        if (page > 0) {
            --page;
        } else {
            page = fd_key_value_store.page_count - 1;
        }
    } while (page != fd_key_value_store.current.page);
    *value_size = 0;
    return NULL;
}

bool fd_key_value_store_has(const char *key) {
    size_t size = 0;
    uint8_t *data = fd_key_value_store_lookup(key, strlen(key), &size);
    if (data == NULL) {
        return false;
    }
    return true;
}

bool fd_key_value_store_get(const char *key, const pb_msgdesc_t *descriptor, void *object) {
    size_t size = 0;
    uint8_t *data = fd_key_value_store_lookup(key, strlen(key), &size);
    if (data == NULL) {
        return false;
    }
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    bool result = pb_decode(&stream, descriptor, object);
    fd_assert(result);
    fd_assert(stream.errmsg == NULL);
    return result;
}

void fd_key_value_store_nvm_erase(uint32_t page) {
    const struct flash_area *area;
    int result = flash_area_open(fd_key_value_store_area_id, &area);
    fd_assert(result == 0);

    result = flash_area_erase(area, page * fd_key_value_store.page_size, fd_key_value_store.page_size);
    fd_assert(result == 0);

    flash_area_close(area);
}

void fd_key_value_store_nvm_write(uint32_t offset, const void *data, size_t count) {
    const struct flash_area *area;
    int result = flash_area_open(fd_key_value_store_area_id, &area);
    fd_assert(result == 0);

    result = flash_area_write(area, offset, data, count);
    fd_assert(result == 0);

    flash_area_close(area);
}

void fd_key_value_store_ostream_write(pb_ostream_t *stream) {
    fd_key_value_store_ostream_state_t *state = stream->state;
    fd_assert(state->count <= sizeof(state->buffer));
    uint32_t offset = fd_key_value_store.current.page * fd_key_value_store.page_size + fd_key_value_store.data_write_offset;
    fd_key_value_store_nvm_write(offset, state->buffer, state->count);
    fd_key_value_store.data_write_offset += state->count;
    state->count = 0;
}

void fd_key_value_store_ostream_flush(pb_ostream_t *stream) {
    fd_key_value_store_ostream_state_t *state = stream->state;
    if (state->count == 0) {
        return;
    }
    state->count = fd_key_value_store_get_write_size(state->count);
    fd_key_value_store_ostream_write(stream);
}

bool fd_key_value_store_ostream_callback(pb_ostream_t *stream, const pb_byte_t *data, size_t count) {
    fd_key_value_store_ostream_state_t *state = stream->state;
    for (size_t i = 0; i < count; i++) {
        if (state->count >= sizeof(state->buffer)) {
            fd_key_value_store_ostream_write(stream);
        }
        state->buffer[state->count++] = data[i];
    }
    return true;
}

pb_ostream_t fd_key_value_store_ostream(void) {
    fd_key_value_store.ostream_state.count = 0;
    return (pb_ostream_t) {
        .callback = fd_key_value_store_ostream_callback,
        .state = &fd_key_value_store.ostream_state,
        .max_size = SIZE_MAX,
    };
}

size_t fd_key_value_store_current_free_bytes(void) {
    size_t used = fd_key_value_store.current.data_byte_count + fd_key_value_store.current.metadata_count * sizeof(fd_key_value_store_metadata_t);
    fd_assert(used <= fd_key_value_store.page_size);
    return fd_key_value_store.page_size - used;
}

void fd_key_value_store_set_commit(size_t key_size, size_t value_size, size_t data_size, uint32_t flags) {
    fd_key_value_store_metadata_t metadata = {
        .key = {
            .offset = fd_key_value_store.current.data_byte_count,
            .size = key_size,
        },
        .value = {
            .offset = fd_key_value_store.current.data_byte_count + key_size,
            .size = value_size,
        },
        .flags = flags,
    };
    fd_key_value_store.current.data_byte_count += data_size;
    ++fd_key_value_store.current.metadata_count;
    uint32_t offset = fd_key_value_store.current.page * fd_key_value_store.page_size + fd_key_value_store.page_size - fd_key_value_store.current.metadata_count * sizeof(fd_key_value_store_metadata_t);
    fd_key_value_store_nvm_write(offset, &metadata, sizeof(metadata));
}

void fd_key_value_store_write(const char *key, size_t key_size, const void *value, size_t value_size, uint32_t flags) {
    fd_key_value_store.data_write_offset = fd_key_value_store.current.data_byte_count;
    pb_ostream_t stream = fd_key_value_store_ostream();
    fd_key_value_store_ostream_callback(&stream, key, key_size);
    fd_key_value_store_ostream_callback(&stream, value, value_size);
    fd_key_value_store_ostream_flush(&stream);
    size_t data_size = fd_key_value_store_get_write_size(key_size + value_size);
    fd_key_value_store_set_commit(key_size, value_size, data_size, flags);
}

void fd_key_value_store_garbage_collect(void) {
    // move all live objects from the first occupied page to the current (first unoccupied) page
    uint32_t page = (fd_key_value_store.current.page + 1) % fd_key_value_store.page_count;
    fd_assert(!fd_key_value_store_is_page_empty(page));
    uint32_t page_address = fd_key_value_store_get_page_address(page);
    uint32_t metadata_count = fd_key_value_store_get_metadata_count(page);
    for (uint32_t i = 0; i < metadata_count; ++i) {
        fd_key_value_store_metadata_t *metadata = fd_key_value_store_get_metadata(page, i);
        if ((metadata->flags & fd_key_value_store_metadata_flags_removed) != 0) {
            continue;
        }

        char *key = (char *)(page_address + metadata->key.offset);
        size_t value_size = 0;
        void *data = fd_key_value_store_lookup_on_page(fd_key_value_store.current.page, key, metadata->key.size, &value_size);
        if (data != NULL) {
            continue;
        }

        data = fd_key_value_store_lookup(key, metadata->key.size, &value_size);
        if (data == NULL) {
            continue;
        }

        char *value = (char *)(page_address + metadata->value.offset);
        fd_key_value_store_write(key, metadata->key.size, value, metadata->value.size, 0);
    }
    fd_key_value_store_nvm_erase(page);
    ++fd_key_value_store.garbage_collection_count;
}

void fd_key_value_store_advance_current_page(void) {
    uint32_t page = (fd_key_value_store.current.page + 1) % fd_key_value_store.page_count;
    fd_assert(fd_key_value_store_is_page_empty(page));
    fd_key_value_store.current.page = page;
    fd_key_value_store.current.data_byte_count = 0;
    fd_key_value_store.current.metadata_count = 0;
}

bool fd_key_value_store_allocate(size_t size) {
    if (fd_key_value_store.is_full) {
        return false;
    }

    int collection_count = 0;
    while (true) {
        size_t free = fd_key_value_store_current_free_bytes();
        if (size <= free) {
            break;
        }
        
        fd_key_value_store_advance_current_page();

        uint32_t skip_page = (fd_key_value_store.current.page + 1) % fd_key_value_store.page_count;
        if (fd_key_value_store_is_page_empty(skip_page)) {
            continue;
        }

        // only one page free, time to garbage collect
        fd_key_value_store_garbage_collect();
        ++collection_count;
        if (collection_count > fd_key_value_store.page_count) {
            fd_key_value_store.is_full = true;
            return false;
        }
    }

    fd_key_value_store.data_write_offset = fd_key_value_store.current.data_byte_count;
    return true;
}

bool fd_key_value_store_allocate_key_value(size_t key_size, size_t value_size, uint32_t *data_size) {
    *data_size = fd_key_value_store_get_write_size(key_size + value_size);
    size_t metadata_end_marker_size = sizeof(fd_key_value_store_metadata_t);
    size_t required = *data_size + metadata_end_marker_size + sizeof(fd_key_value_store_metadata_t);
    bool result = fd_key_value_store_allocate(required);
    fd_assert(result);
    return result;
}

bool fd_key_value_store_add(const char *key, const pb_msgdesc_t *descriptor, const void *object) {
    fd_assert((descriptor == NULL) || (object != NULL));

    size_t key_size = strlen(key);
    size_t value_size = 0;
    if (descriptor != NULL) {
        bool result = pb_get_encoded_size(&value_size, descriptor, object);
        fd_assert(result);
    }
    size_t data_size = 0;
    if (!fd_key_value_store_allocate_key_value(key_size, value_size, &data_size)) {
        return false;
    }

    pb_ostream_t stream = fd_key_value_store_ostream();
    fd_key_value_store_ostream_callback(&stream, key, key_size);

    if (descriptor != NULL) {
        bool result = pb_encode(&stream, descriptor, object);
        fd_assert(result);
        fd_assert(stream.errmsg == NULL);
        fd_assert(stream.bytes_written == value_size);
    }
    fd_key_value_store_ostream_flush(&stream);
    fd_key_value_store_set_commit(key_size, value_size, data_size, (descriptor == NULL) ? fd_key_value_store_metadata_flags_removed : 0);

    for (uint32_t i = 0; i < fd_key_value_store.listener_count; ++i) {
        fd_key_value_store_listener_t *listener = &fd_key_value_store.listeners[i];
        if (descriptor != NULL) {
            if (listener->was_set != NULL) {
                listener->was_set(listener->context, key);
            }
        } else {
            if (listener->was_removed != NULL) {
                listener->was_removed(listener->context, key);
            }
        }
    }

    return true;
}

bool fd_key_value_store_is_set(const char *key) {
    uint32_t value_size = 0;
    return fd_key_value_store_lookup(key, strlen(key), &value_size) != NULL;
}

bool fd_key_value_store_equal_stream_callback(pb_ostream_t *stream, const pb_byte_t *data, size_t count) {
    fd_key_value_store_equal_stream_state_t *state = stream->state;
    for (size_t i = 0; i < count; i++) {
        if (state->count < state->size) {
            if (data[i] != state->value[state->count]) {
                state->is_equal = false;
            }
            ++state->count;
        }
    }
    return true;
}

pb_ostream_t fd_key_value_store_equal_stream(uint8_t *value, size_t size) {
    fd_key_value_store.equal_stream_state.value = value;
    fd_key_value_store.equal_stream_state.size = size;
    fd_key_value_store.equal_stream_state.count = 0;
    fd_key_value_store.equal_stream_state.is_equal = true;
    return (pb_ostream_t) {
        .callback = fd_key_value_store_equal_stream_callback,
        .state = &fd_key_value_store.equal_stream_state,
        .max_size = SIZE_MAX,
    };
}

bool fd_key_value_store_is_set_to(const char *key, const pb_msgdesc_t *descriptor, const void *object) {
    uint32_t value_size = 0;
    void *value = fd_key_value_store_lookup(key, strlen(key), &value_size);
    pb_ostream_t stream = fd_key_value_store_equal_stream(value, value_size);
    bool result = pb_encode(&stream, descriptor, object);
    fd_assert(result);
    return fd_key_value_store.equal_stream_state.is_equal && (stream.bytes_written == value_size);
}

bool fd_key_value_store_set(const char *key, const pb_msgdesc_t *descriptor, const void *object) {
    if (fd_key_value_store_is_set_to(key, descriptor, object)) {
        return true;
    }
    bool result = fd_key_value_store_add(key, descriptor, object);
    if (!result) {
        return false;
    }
    for (uint32_t i = 0; i < fd_key_value_store.listener_count; ++i) {
        fd_key_value_store_listener_t *listener = &fd_key_value_store.listeners[i];
        if (listener->was_set != NULL) {
            listener->was_set(listener->context, key);
        }
    }
    return true;
}

bool fd_key_value_store_remove(const char *key) {
    if (!fd_key_value_store_is_set(key)) {
        return true;
    }
    bool result = fd_key_value_store_add(key, NULL, NULL);
    if (!result) {
        return false;
    }
    for (uint32_t i = 0; i < fd_key_value_store.listener_count; ++i) {
        fd_key_value_store_listener_t *listener = &fd_key_value_store.listeners[i];
        if (listener->was_removed != NULL) {
            listener->was_removed(listener->context, key);
        }
    }
    return true;
}

void fd_key_value_store_erase(void) {
    for (uint32_t i = 0; i < fd_key_value_store.page_count; ++i) {
        fd_key_value_store_nvm_erase(i);
    }
    fd_key_value_store_update_current();

    for (uint32_t i = 0; i < fd_key_value_store.listener_count; ++i) {
        fd_key_value_store_listener_t *listener = &fd_key_value_store.listeners[i];
        if (listener->was_erased != NULL) {
            listener->was_erased(listener->context);
        }
    }
}

//#define FD_KEY_VALUE_STORE_TEST
#ifdef FD_KEY_VALUE_STORE_TEST

#include "protobuf/rtc.pb.h"

void fd_key_value_store_test_basic(void) {
    uint32_t original_page_count = fd_key_value_store.page_count;
    fd_key_value_store.page_count = 2;
    fd_key_value_store_erase();

    fd_key_value_store_update_current();
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 0);
    fd_assert(fd_key_value_store.current.metadata_count == 0);

    const char *key = "conf";
    pison_rtc_v1_Configuration configuration = {
        .display_format = 1,
        .time_zone_offset = 2,
    };
    size_t key_size = strlen(key);
    size_t value_size = 4;
    bool result = fd_key_value_store_set(key, &pison_rtc_v1_Configuration_msg, &configuration);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == key_size + value_size);
    fd_assert(fd_key_value_store.current.metadata_count == 1);

    fd_key_value_store_update_current();
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == key_size + value_size);
    fd_assert(fd_key_value_store.current.metadata_count == 1);
    pison_rtc_v1_Configuration verify = {};
    result = fd_key_value_store_get(key, &pison_rtc_v1_Configuration_msg, &verify);
    fd_assert(result);
    fd_assert(configuration.display_format == verify.display_format);
    fd_assert(configuration.time_zone_offset == verify.time_zone_offset);

    result = fd_key_value_store_remove(key);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 2 * key_size + value_size);
    fd_assert(fd_key_value_store.current.metadata_count == 2);

    result = fd_key_value_store_get(key, &pison_rtc_v1_Configuration_msg, &verify);
    fd_assert(!result);

    fd_key_value_store_advance_current_page();
    fd_key_value_store_garbage_collect();
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 1);
    fd_assert(fd_key_value_store.current.data_byte_count == 0);
    fd_assert(fd_key_value_store.current.metadata_count == 0);

    fd_key_value_store_update_current();
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 0);
    fd_assert(fd_key_value_store.current.metadata_count == 0);

    fd_key_value_store.page_count = original_page_count;
    fd_key_value_store_erase();
}

void fd_key_value_store_test_noop(void) {
    uint32_t original_page_count = fd_key_value_store.page_count;
    fd_key_value_store.page_count = 2;
    fd_key_value_store_erase();

    fd_key_value_store_update_current();
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 0);
    fd_assert(fd_key_value_store.current.metadata_count == 0);

    const char *key = "conf";
    pison_rtc_v1_Configuration configuration = {
        .display_format = 1,
        .time_zone_offset = 2,
    };
    size_t key_size = strlen(key);
    size_t value_size = 4;
    bool result = fd_key_value_store_set(key, &pison_rtc_v1_Configuration_msg, &configuration);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == key_size + value_size);
    fd_assert(fd_key_value_store.current.metadata_count == 1);

    result = fd_key_value_store_set(key, &pison_rtc_v1_Configuration_msg, &configuration);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == key_size + value_size);
    fd_assert(fd_key_value_store.current.metadata_count == 1);

    configuration.time_zone_offset = 3;
    result = fd_key_value_store_set(key, &pison_rtc_v1_Configuration_msg, &configuration);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 2 * (key_size + value_size));
    fd_assert(fd_key_value_store.current.metadata_count == 2);

    result = fd_key_value_store_remove(key);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 3 * key_size + 2 * value_size);
    fd_assert(fd_key_value_store.current.metadata_count == 3);

    result = fd_key_value_store_remove(key);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 3 * key_size + 2 * value_size);
    fd_assert(fd_key_value_store.current.metadata_count == 3);

    fd_key_value_store.page_count = original_page_count;
    fd_key_value_store_erase();
}

void fd_key_value_store_test_collect(void) {
    uint32_t original_page_count = fd_key_value_store.page_count;
    fd_key_value_store.page_count = 2;
    fd_key_value_store_erase();

    fd_key_value_store_update_current();

    const char *key = "conf";
    pison_rtc_v1_Configuration configuration = {
        .display_format = 1,
        .time_zone_offset = 2,
    };
    for (int i = 0; i < 1024; ++i) {
        configuration.time_zone_offset += 1;
        bool result = fd_key_value_store_set(key, &pison_rtc_v1_Configuration_msg, &configuration);
        fd_assert(result);
        fd_assert(!fd_key_value_store.is_full);
    }

    bool result = fd_key_value_store_remove(key);
    fd_assert(result);
    fd_assert(!fd_key_value_store.is_full);

    result = fd_key_value_store_get(key, &pison_rtc_v1_Configuration_msg, &configuration);
    fd_assert(!result);

    fd_key_value_store_advance_current_page();
    fd_key_value_store_garbage_collect();
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.data_byte_count == 0);
    fd_assert(fd_key_value_store.current.metadata_count == 0);

    fd_key_value_store_update_current();
    fd_assert(!fd_key_value_store.is_full);
    fd_assert(fd_key_value_store.current.page == 0);
    fd_assert(fd_key_value_store.current.data_byte_count == 0);
    fd_assert(fd_key_value_store.current.metadata_count == 0);

    fd_key_value_store.page_count = original_page_count;
    fd_key_value_store_erase();
}

#endif

void fd_key_value_store_initialize(void) {
    fd_key_value_store.base_address = DT_REG_ADDR(DT_NODELABEL(persist_partition)); // DT_PATH(flash0, partitions, persist_partition));
    fd_key_value_store.minumum_write_size = 4; // !!! where to get this from dts? -denis
    fd_key_value_store.page_size = 4096; // !!! where to get this from dts? -denis
    struct flash_sector sectors[10]; // !!! limits to 10 sectors -denis
    uint32_t count = ARRAY_SIZE(sectors);
    int result = flash_area_get_sectors(fd_key_value_store_area_id, &count, sectors);
    fd_assert(result == 0);
    fd_key_value_store.page_count = count;
    fd_assert(fd_key_value_store.page_count >= 2);

#ifdef FD_KEY_VALUE_STORE_TEST
    fd_key_value_store_test_basic();
    fd_key_value_store_test_noop();
    fd_key_value_store_test_collect();
#endif

   fd_key_value_store_update_current();
}
