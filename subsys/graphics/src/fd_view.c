#include "fd_view.h"

#include "fd_assert.h"

#include <stdio.h>
#include <string.h>

void fd_view_string_update_reference(fd_view_resource_string_t *resource, const char *string) {
    fd_assert(resource->size == 0);

    if (resource->string == string) {
        return;
    }

    resource->string = string;
    resource->length = strlen(string);
    ++resource->revision;
}

void fd_view_string_update(fd_view_resource_string_t *resource, const char *string) {
    fd_assert(resource->size > 0);
    
    size_t length = strlen(string);
    if ((resource->length == length) && (memcmp(resource->string, string, length) == 0)) {
        return;
    }
    
    memcpy((char *)resource->string, string, length);
    resource->length = length;
    ++resource->revision;
}

void fd_view_string_update_int_suffix(fd_view_resource_string_t *resource, int value, const char *suffix) {
    char buffer[8];
    size_t size = sizeof(buffer);
    if (resource->size < size) {
        size = resource->size;
    }
    if (suffix == 0) {
        suffix = "";
    }
    snprintf(buffer, size, "%d%s", value, suffix);

    fd_view_string_update(resource, buffer);
}

void fd_view_string_update_int(fd_view_resource_string_t *resource, int value) {
    fd_view_string_update_int_suffix(resource, value, 0);
}
