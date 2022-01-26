#ifndef fd_boot_h
#define fd_boot_h

/*
Storage Formats:
- data is little endian
- hash is SHA-1
- encryption is AES-128 CBC


Executable Format:

N bytes of executable data
  Typically this is the ARM Cortex interrupt vector.
  Must be a multiple of 64-bytes.

64 + addendum length bytes of metadata:
  uint32_t header magic
  uint32_t header version
  uint32_t version major
  uint32_t version minor
  uint32_t version patch
  uint32_t length // length of the executable
  uint8_t [20] hash // hash of this executable (calculated with this hash as zero)
  uint32_t addendum_length // length of the metadata addendum in bytes (must be multiple of 64-bytes)
  uint8_t [16] reserved
  addendum bytes // reserved for future use

M bytes of executable data:
  Typically this is the rest of the binary (other than the interrupt vector).

P bytes of padding so that the executable length is a multiple of 64 bytes

4 bytes of trailer:
  uint32_t magic


Update Format:

128 + addendum length bytes of metadata:
  uint32_t header magic
  uint32_t header version
  uint32_t executable version major
  uint32_t executable version minor
  uint32_t executable version patch
  uint32_t executable length // length of the executable
  uint8_t [20] executable hash // hash of executable
  uint8_t [20] hash // hash of this update (calculated with this hash as zero)
  uint32_t flags;
  uint8_t [16] initialization vector // decryption initialization vector
  uint32_t addendum_length // length of the metadata addendum in bytes (must be multiple of 64-bytes)
  uint8_t [40] reserved
  addendum bytes // reserved for future use

128 + addendum length bytes of encrypted metadata // only present if the update is encrypted

N bytes of executable data // encrypted if the update is encrypted

P bytes of padding so that the update length is a multiple of 64 bytes

4 bytes of trailer:
  uint32_t magic
*/

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t code;
} fd_boot_error_t;

void fd_boot_set_error(fd_boot_error_t *error, uint32_t code);

typedef struct {
    uint32_t location;
    uint32_t length;
} fd_boot_range_t;

#define FD_BOOT_HASH_DATA_SIZE 20

typedef struct {
    uint8_t data[FD_BOOT_HASH_DATA_SIZE];
} fd_boot_hash_t;

#define FD_BOOT_HASH_CONTEXT_SIZE 128

typedef struct {
    uint8_t data[FD_BOOT_HASH_CONTEXT_SIZE];
} fd_boot_hash_context_t;

#define FD_BOOT_HASH_BLOCK_SIZE 64

typedef struct {
    bool (*initialize)(fd_boot_hash_context_t *context, fd_boot_error_t *error);
    bool (*update)(fd_boot_hash_context_t *context, const uint8_t *data, uint32_t length, fd_boot_error_t *error);
    bool (*finalize)(fd_boot_hash_context_t *context, fd_boot_hash_t *hash, fd_boot_error_t *error);
} fd_boot_hash_interface_t;

#define FD_BOOT_CRYPTO_KEY_SIZE 16

typedef struct {
    uint8_t data[FD_BOOT_CRYPTO_KEY_SIZE];
} fd_boot_crypto_key_t;

#define FD_BOOT_CRYPTO_INITIALIZATION_VECTOR_SIZE 16

typedef struct {
    uint8_t data[FD_BOOT_CRYPTO_INITIALIZATION_VECTOR_SIZE];
} fd_boot_crypto_initialization_vector_t;

#define FD_BOOT_DECRYPT_CONTEXT_SIZE 16

typedef struct {
    uint8_t data[FD_BOOT_DECRYPT_CONTEXT_SIZE];
} fd_boot_decrypt_context_t;

#define FD_BOOT_DECRYPT_BLOCK_SIZE 16

typedef struct {
    bool (*initialize)(fd_boot_decrypt_context_t *decrypt, const fd_boot_crypto_key_t *key, const fd_boot_crypto_initialization_vector_t *initialization_vector, fd_boot_error_t *error);
    bool (*update)(fd_boot_decrypt_context_t *decrypt, const uint8_t *in, uint8_t *out, uint32_t length, fd_boot_error_t *error);
    bool (*finalize)(fd_boot_decrypt_context_t *decrypt, fd_boot_error_t *error);
} fd_boot_decrypt_interface_t;

typedef struct {
    void *context;
    bool (*read)(void *context, uint32_t location, uint8_t *data, uint32_t length, fd_boot_error_t *error);
} fd_boot_reader_interface_t;

typedef struct {
    void *context;
    bool (*initialize)(void *context, uint32_t location, uint32_t length, fd_boot_error_t *error);
    bool (*write)(void *context, uint32_t location, const uint8_t *data, uint32_t length, fd_boot_error_t *error);
    bool (*finalize)(void *context, fd_boot_error_t *error);
} fd_boot_flasher_interface_t;

typedef struct {
    fd_boot_range_t range;
} fd_boot_info_executable_storage_t;

typedef struct {
    uint32_t location;
    uint32_t metadata_offset;
} fd_boot_info_executable_t;

typedef struct {
    fd_boot_range_t range;
} fd_boot_info_update_storage_t;

typedef struct {
    fd_boot_crypto_key_t key;
} fd_boot_info_decryption_t;

typedef struct {
    bool (*get_executable_storage)(fd_boot_info_executable_storage_t *storage, fd_boot_error_t *error);
    bool (*get_executable)(fd_boot_info_executable_t *executable, fd_boot_error_t *error);
    bool (*get_update_storage)(fd_boot_info_update_storage_t *storage, fd_boot_error_t *error);
    bool (*get_decryption)(fd_boot_info_decryption_t *decryption, fd_boot_error_t *error);
} fd_boot_info_interface_t;

typedef struct {
     void (*feed)(void);
} fd_boot_watchdog_interface_t;

typedef struct {
    void (*progress)(float amount);
} fd_boot_progress_interface_t;

typedef struct __attribute__((packed)) {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} fd_boot_version_t;

#define fd_boot_executable_metadata_header_magic 0xb001da1a
#define fd_boot_executable_metadata_header_version 1
#define fd_boot_executable_trailer_magic 0xb001e00d

typedef struct __attribute__((packed)) {
    fd_boot_version_t version;
    uint32_t length;
    fd_boot_hash_t hash;
} fd_boot_executable_metadata_t;

#define fd_boot_update_metadata_header_magic 0xab09da1e
#define fd_boot_update_metadata_header_version 1
#define fd_boot_update_trailer_magic 0xda1ee00d

#define fd_boot_update_metadata_flag_encrypted 0x0001

typedef struct __attribute__((packed)) {
    fd_boot_executable_metadata_t executable_metadata;
    fd_boot_hash_t hash;
    uint32_t flags;
    fd_boot_crypto_initialization_vector_t initialization_vector;
} fd_boot_update_metadata_t;

typedef struct {
    bool (*can_upgrade)(const fd_boot_version_t *executable_version, const fd_boot_version_t *update_version);
    bool (*can_downgrade)(const fd_boot_version_t *executable_version, const fd_boot_version_t *update_version);
    bool (*can_install)(const fd_boot_version_t *version);
} fd_boot_action_interface_t;

typedef struct {
    bool (*cleanup)(fd_boot_error_t *error);
    bool (*start)(uint32_t address, fd_boot_error_t *error);
} fd_boot_executor_interface_t;

typedef struct {
    fd_boot_info_interface_t info;
    fd_boot_progress_interface_t progress;
    fd_boot_watchdog_interface_t watchdog;
    fd_boot_hash_interface_t hash;
    fd_boot_decrypt_interface_t decrypt;
    fd_boot_reader_interface_t executable_reader;
    fd_boot_flasher_interface_t executable_flasher;
    fd_boot_reader_interface_t update_reader;
    fd_boot_action_interface_t action;
    fd_boot_executor_interface_t executor;
} fd_boot_update_interface_t;

typedef enum {
    fd_boot_update_issue_firmware,
    fd_boot_update_issue_update,
} fd_boot_update_issue_t;

typedef struct {
    bool is_valid;
    fd_boot_update_issue_t issue;
} fd_boot_update_result_t;

bool fd_boot_update(
    fd_boot_update_interface_t *interface,
    fd_boot_update_result_t *result,
    fd_boot_error_t *error
);

bool fd_boot_execute(
    fd_boot_update_interface_t *interface,
    fd_boot_error_t *error
);

#endif
