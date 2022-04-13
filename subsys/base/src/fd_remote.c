#include "fd_remote.h"

#include <string.h>

bool fd_remote_copy_string(fd_binary_string_t source, char *destination, size_t size) {
    if (size == 0) {
        return false;
    }
    if (source.length >= size) {
        memcpy(destination, source.data, size - 1);
        destination[size - 1] = '\0';
        return false;
    }
    memcpy(destination, source.data, source.length);
    destination[source.length] = '\0';
    return true;
}
