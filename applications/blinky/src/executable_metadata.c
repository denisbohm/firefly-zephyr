#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    uint32_t length;
    uint8_t hash[20];
    uint8_t reserved[20];
} executable_metadata_t;

__attribute__((section(".executable_metadata_section")))
executable_metadata_t executable_metadata = {
    .magic = 0xb001da1a,
    .version = 1,
    .major = 1,
    .minor = 2,
    .patch = 3,
};
