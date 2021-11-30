#include "fd_assert.h"
#include "fd_cobs.h"
#include "fd_delay.h"
#include "fd_log.h"
#include "fd_rtc.h"

#include <string.h>

void cobs_test(uint8_t *data, size_t length) {
    uint8_t work[512];
    memcpy(work, data, length);
    uint8_t buffer[32];
    size_t encoded_length = fd_cobs_encode(work, length, sizeof(work), buffer, sizeof(buffer));
    for (size_t i = 0; i < encoded_length; ++i) {
        fd_assert(work[i] != 0);
    }
    size_t decoded_length = fd_cobs_decode(work, encoded_length);
    fd_assert(decoded_length == length);
}

void cobs_tests(void) {
    uint8_t test_a[] = { };
    cobs_test(test_a, sizeof(test_a));

    uint8_t test_b[] = { 0 };
    cobs_test(test_b, sizeof(test_b));

    uint8_t test_c[] = { 1, 2, 0, 3 };
    cobs_test(test_c, sizeof(test_c));

    uint8_t test_z[300];
    for (int i = 0; i < sizeof(test_z); ++i) {
        test_z[i] = (uint8_t)(i & 0xff);
    }
    cobs_test(test_z, sizeof(test_z));
}

void main(void) {
    cobs_tests();

    fd_log_initialize();
    fd_rtc_initialize();
    fd_rtc_set_utc(1636381101); // 8:18 AM CST
    while (1) {
        uint64_t utc = fd_rtc_get_utc();
        fd_delay_ms(5000);
    }
}
