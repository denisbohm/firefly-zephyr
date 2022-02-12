#include "fd_version.h"

bool fd_version_is_lt(const fd_version_t *a, const fd_version_t *b) {
    if (a->major < b->major) {
        return true;
    }
    if (a->major > b->major) {
        return false;
    }
    if (a->minor < b->minor) {
        return true;
    }
    if (a->minor > b->minor) {
        return false;
    }
    return a->patch < b->patch;
}
