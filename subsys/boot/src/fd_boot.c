#include "fd_boot.h"

#include <string.h>

void fd_boot_set_error(fd_boot_error_t *error, uint32_t code) {
    error->code = code;
}

bool fd_boot_is_valid_subrange(fd_boot_range_t range, fd_boot_range_t subrange) {
    return (range.location <= subrange.location) && ((subrange.location + subrange.length) <= (range.location + range.length));
}

typedef enum {
    fd_boot_check_hash_issue_mismatch,
} fd_boot_check_hash_issue_t;

typedef struct {
    bool is_valid;
    fd_boot_check_hash_issue_t issue;
} fd_boot_check_hash_result_t;

bool fd_boot_check_hash(
    fd_boot_update_interface_t *interface,
    fd_boot_reader_interface_t *reader,
    fd_boot_range_t storage_range,
    fd_boot_range_t range,
    fd_boot_range_t zero,
    fd_boot_hash_t *hash,
    fd_boot_check_hash_result_t *result,
    fd_boot_error_t *error
) {
    memset(result, 0, sizeof(*result));

    if (!fd_boot_is_valid_subrange(storage_range, range)) {
        fd_boot_set_error(error, 1);
        return false;
    }

    fd_boot_hash_context_t hash_context;
    if (!interface->hash.initialize(&hash_context, error)) {
        return false;
    }
    uint32_t location = range.location;
    uint32_t end = range.location + range.length;
    while (location < end) {
        interface->watchdog.feed();
        
        uint32_t length = end - location;
        if (length > FD_BOOT_HASH_BLOCK_SIZE) {
            length = FD_BOOT_HASH_BLOCK_SIZE;
        }
        uint8_t data[FD_BOOT_HASH_BLOCK_SIZE];
        if (!reader->read(reader->context, location, data, length, error)) {
            return false;
        }
        if ((location <= zero.location) && ((zero.location + zero.length) <= (location + length))) {
            memset(&data[zero.location - location], 0, zero.length);
        }
        if (!interface->hash.update(&hash_context, data, length, error)) {
            return false;
        }

        location += length;
    }
    fd_boot_hash_t actual_hash;
    if (!interface->hash.finalize(&hash_context, &actual_hash, error)) {
        return false;
    }
    if (memcmp(actual_hash.data, hash->data, sizeof(hash->data)) != 0) {
        result->issue = fd_boot_check_hash_issue_mismatch;
        return true;
    }

    result->is_valid = true;
    return true;
}

typedef struct {
    uint32_t magic;
    uint32_t version;
} fd_boot_executable_metadata_header_t;

typedef struct {
    uint32_t magic;
} fd_boot_executable_trailer_t;

typedef enum {
    fd_boot_get_executable_metadata_issue_trailer_magic,
    fd_boot_get_executable_metadata_issue_header_magic,
    fd_boot_get_executable_metadata_issue_header_version,
    fd_boot_get_executable_metadata_issue_length,
    fd_boot_get_executable_metadata_issue_hash,
} fd_boot_get_executable_metadata_issue_t;

typedef struct {
    bool is_valid;
    fd_boot_get_executable_metadata_issue_t issue;
    fd_boot_executable_metadata_t metadata;
} fd_boot_get_executable_metadata_result_t;

bool fd_boot_get_executable_metadata(
    fd_boot_update_interface_t *interface,
    fd_boot_get_executable_metadata_result_t *result,
    fd_boot_error_t *error
) {
    memset(result, 0, sizeof(*result));

    fd_boot_info_executable_storage_t info_storage;
    if (!interface->info.get_executable_storage(&info_storage, error)) {
        return false;
    }
    fd_boot_info_executable_t info_executable;
    if (!interface->info.get_executable(&info_executable, error)) {
        return false;
    }

    fd_boot_executable_trailer_t trailer;
    fd_boot_range_t trailer_range = {
        .location = info_executable.range.location + info_executable.range.length - sizeof(trailer),
        .length = sizeof(trailer),
    };
    if (!fd_boot_is_valid_subrange(info_storage.range, trailer_range)) {
        fd_boot_set_error(error, 1);
        return false;
    }
    if (!interface->executable_reader.read(
        interface->executable_reader.context,
        trailer_range.location,
        (uint8_t *)&trailer,
        trailer_range.length,
        error
    )) {
        return false;
    }
    if (trailer.magic != fd_boot_executable_trailer_magic) {
        result->issue = fd_boot_get_executable_metadata_issue_trailer_magic;
        return true;
    }

    fd_boot_range_t metadata_header_range = {
        .location = info_executable.range.location + info_executable.metadata_offset,
        .length = sizeof(fd_boot_executable_metadata_header_t),
    };
    if (!fd_boot_is_valid_subrange(info_storage.range, metadata_header_range)) {
        fd_boot_set_error(error, 1);
        return false;
    }
    fd_boot_executable_metadata_header_t metadata_header;
    if (!interface->executable_reader.read(
        interface->executable_reader.context,
        metadata_header_range.location,
        (uint8_t *)&metadata_header,
        metadata_header_range.length,
        error
    )) {
        return false;
    }
    if (metadata_header.magic != fd_boot_executable_metadata_header_magic) {
        result->issue = fd_boot_get_executable_metadata_issue_header_magic;
        return true;
    }
    if (metadata_header.version != fd_boot_executable_metadata_header_version) {
        result->issue = fd_boot_get_executable_metadata_issue_header_version;
        return true;
    }

    fd_boot_range_t metadata_range = {
        .location =
            info_executable.range.location +
            info_executable.metadata_offset +
            sizeof(fd_boot_executable_metadata_header_t),
        .length = sizeof(fd_boot_executable_metadata_t),
    };
    if (!fd_boot_is_valid_subrange(info_storage.range, metadata_range)) {
        fd_boot_set_error(error, 1);
        return false;
    }
    fd_boot_executable_metadata_t metadata;
    if (!interface->executable_reader.read(
        interface->executable_reader.context,
        metadata_range.location,
        (uint8_t *)&metadata,
        metadata_range.length,
        error
    )) {
        return false;
    }
    if ((metadata.length % FD_BOOT_HASH_BLOCK_SIZE) != 0) {
        result->issue = fd_boot_get_executable_metadata_issue_length;
        return false;
    }

    fd_boot_range_t executable_range = {
        .location = info_executable.range.location,
        .length = metadata.length,
    };
    fd_boot_range_t hash_range = {
        .location = executable_range.location + info_executable.metadata_offset + 24,
        .length = FD_BOOT_HASH_DATA_SIZE,
    };
    fd_boot_check_hash_result_t hash_result;
    if (!fd_boot_check_hash(
        interface,
        &interface->executable_reader,
        info_storage.range,
        executable_range,
        hash_range,
        &metadata.hash,
        &hash_result,
        error
    )) {
        return false;
    }
    if (!hash_result.is_valid) {
        result->issue = fd_boot_get_executable_metadata_issue_hash;
        return true;
    }

    result->is_valid = true;
    memcpy(&result->metadata, &metadata, sizeof(metadata));
    return true;
}

typedef struct {
    fd_boot_reader_interface_t *reader;
    fd_boot_decrypt_context_t decrypt_context;
    fd_boot_decrypt_interface_t decrypt_interface;
} fd_boot_read_and_decrypt_context_t;

bool fd_boot_read_and_decrypt(
    void *anonymous_context,
    uint32_t location,
    uint8_t *data,
    uint32_t length,
    fd_boot_error_t *error
) {
    fd_boot_read_and_decrypt_context_t *context = (fd_boot_read_and_decrypt_context_t *)anonymous_context;
    if (!context->reader->read(context->reader->context, location, data, length, error)) {
        return false;
    }
    if (length != FD_BOOT_HASH_BLOCK_SIZE) {
        fd_boot_set_error(error, 1);
        return false;
    }
    uint8_t out[FD_BOOT_HASH_BLOCK_SIZE];
    if (!context->decrypt_interface.update(&context->decrypt_context, data, out, length, error)) {
        return false;
    }
    memcpy(data, out, FD_BOOT_DECRYPT_BLOCK_SIZE);
    return true;
}

bool fd_boot_decrypt_range(
    fd_boot_update_interface_t *interface,
    fd_boot_update_metadata_t *metadata,
    fd_boot_range_t range,
    uint8_t *data,
    fd_boot_error_t *error
) {
    fd_boot_info_decryption_t decryption;
    if (!interface->info.get_decryption(&decryption, error)) {
        return false;
    }
    fd_boot_decrypt_context_t decrypt_context;
    if (!interface->decrypt.initialize(
        &decrypt_context,
        &decryption.key,
        &metadata->initialization_vector,
        error
    )) {
        return false;
    }
    uint32_t location = range.location;
    uint32_t end = range.location + range.length;
    uint32_t data_offset = 0;
    while (location < end) {
        interface->watchdog.feed();
        
        uint32_t length = end - location;
        if (length > FD_BOOT_DECRYPT_BLOCK_SIZE) {
            length = FD_BOOT_DECRYPT_BLOCK_SIZE;
        }
        uint8_t in[FD_BOOT_DECRYPT_BLOCK_SIZE];
        if (!interface->update_reader.read(
            interface->update_reader.context,
            location,
            in,
            length,
            error
        )) {
            return false;
        }
        if (!interface->decrypt.update(&decrypt_context, in, &data[data_offset], length, error)) {
            return false;
        }
        location += length;
        data_offset += length;
    }
    if (!interface->decrypt.finalize(&decrypt_context, error)) {
        return false;
    }
    return true;
}

typedef struct {
    uint32_t magic;
    uint32_t version;
} fd_boot_update_metadata_header_t;

typedef enum {
    fd_boot_get_update_metadata_issue_header_magic,
    fd_boot_get_update_metadata_issue_header_version,
    fd_boot_get_update_metadata_issue_length,
    fd_boot_get_update_metadata_issue_hash,
    fd_boot_get_update_metadata_issue_metadata,
} fd_boot_get_update_metadata_issue_t;

typedef struct {
    bool is_valid;
    fd_boot_get_update_metadata_issue_t issue;
    fd_boot_update_metadata_t metadata;
} fd_boot_get_update_metadata_result_t;

bool fd_boot_get_update_metadata(
    fd_boot_update_interface_t *interface,
    fd_boot_get_update_metadata_result_t *result,
    fd_boot_error_t *error
) {
    memset(result, 0, sizeof(*result));

    fd_boot_info_update_storage_t info_storage;
    if (!interface->info.get_update_storage(&info_storage, error)) {
        return false;
    }

    fd_boot_range_t metadata_header_range = {
        .location = info_storage.range.location,
        .length = sizeof(fd_boot_update_metadata_header_t),
    };
    if (!fd_boot_is_valid_subrange(info_storage.range, metadata_header_range)) {
        fd_boot_set_error(error, 1);
        return false;
    }
    fd_boot_update_metadata_header_t metadata_header;
    if (!interface->update_reader.read(
        interface->update_reader.context,
        metadata_header_range.location,
        (uint8_t *)&metadata_header,
        metadata_header_range.length,
        error
    )) {
        fd_boot_set_error(error, 1);
        return false;
    }
    if (metadata_header.magic != fd_boot_update_metadata_header_magic) {
        result->issue = fd_boot_get_update_metadata_issue_header_magic;
        return true;
    }
    if (metadata_header.version != fd_boot_update_metadata_header_version) {
        result->issue = fd_boot_get_update_metadata_issue_header_version;
        return true;
    }

    fd_boot_range_t metadata_range = {
        .location = info_storage.range.location + sizeof(fd_boot_update_metadata_header_t),
        .length = sizeof(fd_boot_update_metadata_t),
    };
    if (!fd_boot_is_valid_subrange(info_storage.range, metadata_range)) {
        fd_boot_set_error(error, 1);
        return false;
    }
    fd_boot_update_metadata_t metadata;
    if (!interface->update_reader.read(
        interface->update_reader.context,
        metadata_range.location,
        (uint8_t *)&metadata,
        metadata_range.length,
        error
    )) {
        return false;
    }
    if ((metadata.executable_metadata.length % FD_BOOT_HASH_BLOCK_SIZE) != 0) {
        result->issue = fd_boot_get_update_metadata_issue_length;
        return true;
    }

    if ((metadata.flags & fd_boot_update_metadata_flag_encrypted) == 0) {
        // update is not encyrpted...

        // check hash
        uint32_t header_length = sizeof(fd_boot_update_metadata_header_t) + sizeof(fd_boot_update_metadata_t) + 0;
        fd_boot_range_t executable_range = {
            .location = info_storage.range.location + header_length,
            .length = metadata.executable_metadata.length,
        };
        fd_boot_range_t hash_range = {
            .location = 0, // ???
            .length = FD_BOOT_HASH_DATA_SIZE,
        };
        fd_boot_check_hash_result_t hash_result;
        if (!fd_boot_check_hash(
            interface,
            &interface->update_reader,
            info_storage.range,
            executable_range,
            hash_range,
            &metadata.executable_metadata.hash,
            &hash_result,
            error
        )) {
            return false;
        }
        if (!hash_result.is_valid) {
            result->issue = fd_boot_get_update_metadata_issue_hash;
            return true;
        }

        result->is_valid = true;
        memcpy(&result->metadata, &metadata, sizeof(metadata));
        return true;
    }

    // update is encrypted

    // check update hash
    fd_boot_check_hash_result_t hash_result;
    fd_boot_range_t hash_range = {
        .location = info_storage.range.location + 44,
        .length = FD_BOOT_HASH_DATA_SIZE,
    };
    if (!fd_boot_check_hash(
        interface,
        &interface->update_reader,
        info_storage.range,
        info_storage.range,
        hash_range,
        &metadata.hash,
        &hash_result,
        error
    )) {
        return false;
    }
    if (!hash_result.is_valid) {
        result->issue = fd_boot_get_update_metadata_issue_hash;
        return true;
    }

    // get decrypted metadata
    fd_boot_range_t range = {
        .location = 128,
        .length = 128,
    };
    uint8_t decrypted[128];
    if (!fd_boot_decrypt_range(interface, &metadata, range, decrypted, error)) {
        return false;
    }
    fd_boot_update_metadata_t decrypted_metadata;
    memcpy(&decrypted_metadata, &decrypted[8], sizeof(decrypted_metadata));
    fd_boot_update_metadata_t expected_metadata;
    memcpy((uint8_t *)&expected_metadata, &metadata, sizeof(metadata));
    memset(&((uint8_t *)&expected_metadata)[36], 0, FD_BOOT_HASH_DATA_SIZE);
    if (memcmp(&expected_metadata, &decrypted_metadata, sizeof(metadata)) != 0) {
        result->issue = fd_boot_get_update_metadata_issue_metadata;
        return true;
    }

    // check deencrypted executable hash
    fd_boot_info_decryption_t decryption;
    if (!interface->info.get_decryption(&decryption, error)) {
        return false;
    }
    fd_boot_decrypt_context_t decrypt_context;
    if (!interface->decrypt.initialize(
        &decrypt_context,
        &decryption.key,
        &metadata.initialization_vector,
        error
    )) {
        return false;
    }
    fd_boot_read_and_decrypt_context_t context = {
        .reader = &interface->update_reader,
        .decrypt_interface = interface->decrypt,
    };
    fd_boot_reader_interface_t reader = {
        .context = &context,
        .read = fd_boot_read_and_decrypt,
    };
    fd_boot_range_t executable_range = {
        .location = info_storage.range.location + info_storage.range.length - 64 - metadata.executable_metadata.length,
        .length = metadata.executable_metadata.length,
    };
    fd_boot_info_executable_t info_executable;
    if (!interface->info.get_executable(&info_executable, error)) {
        return false;
    }
    fd_boot_range_t executable_hash_range = {
        .location = executable_range.location + info_executable.metadata_offset + 24,
        .length = FD_BOOT_HASH_DATA_SIZE,
    };
    if (!fd_boot_check_hash(
        interface,
        &reader,
        info_storage.range,
        executable_range,
        executable_hash_range,
        &metadata.executable_metadata.hash,
        &hash_result,
        error
    )) {
        return false;
    }
    if (!interface->decrypt.finalize(&context.decrypt_context, error)) {
        return false;
    }
    if (!hash_result.is_valid) {
        result->issue = fd_boot_get_update_metadata_issue_hash;
        return true;
    }

    result->is_valid = true;
    result->issue = 0;
    memcpy(&result->metadata, &metadata, sizeof(metadata));
    return true;
}

bool fd_boot_version_is_eq(const fd_boot_version_t *a, const fd_boot_version_t *b) {
    return (a->major == b->major) && (a->minor == b->minor) && (a->patch == b->patch);
}

bool fd_boot_version_is_lt(const fd_boot_version_t *a, const fd_boot_version_t *b) {
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

typedef enum {
    fd_boot_update_action_none, // no existing executable
    fd_boot_update_action_run,
    fd_boot_update_action_install,
} fd_boot_update_action_t;

fd_boot_update_action_t fd_boot_get_action(
    fd_boot_action_interface_t *interface,
    fd_boot_get_executable_metadata_result_t *executable,
    fd_boot_get_update_metadata_result_t *update
) {
    if (executable->is_valid && update->is_valid) {
        const fd_boot_version_t *executable_version = &executable->metadata.version;
        const fd_boot_version_t *update_version = &update->metadata.executable_metadata.version;
        if (fd_boot_version_is_eq(update_version, executable_version)) {
            return fd_boot_update_action_run;
        } else
        if (fd_boot_version_is_lt(executable_version, update_version)) {
            if (interface->can_upgrade(executable_version, update_version)) {
                return fd_boot_update_action_install;
            } else {
                return fd_boot_update_action_run;
            }
        } else {
            if (interface->can_downgrade(executable_version, update_version)) {
                return fd_boot_update_action_install;
            } else {
                return fd_boot_update_action_run;
            }
        }
    } else
    if (executable->is_valid && !update->is_valid) {
        return fd_boot_update_action_run;
    } else
    if (!executable->is_valid && update->is_valid) {
        if (interface->can_install(&update->metadata.executable_metadata.version)) {
            return fd_boot_update_action_install;
        } else {
            return fd_boot_update_action_none;
        }
    } else {
        return fd_boot_update_action_none;
    }
}

bool fd_boot_install(
    fd_boot_update_interface_t *interface,
    fd_boot_update_metadata_t *metadata,
    fd_boot_error_t *error
) {
    fd_boot_info_decryption_t decryption;
    if (!interface->info.get_decryption(&decryption, error)) {
        return false;
    }
    fd_boot_decrypt_context_t decrypt_context;
    if (!interface->decrypt.initialize(
        &decrypt_context,
        &decryption.key,
        &metadata->initialization_vector,
        error
    )) {
        return false;
    }
    if (!interface->executable_flasher.initialize(
        interface->executable_flasher.context,
        0,
        metadata->executable_metadata.length,
        error
    )) {
        return false;
    }


    uint32_t executable_length = metadata->executable_metadata.length;
    uint32_t block_count = executable_length / FD_BOOT_HASH_BLOCK_SIZE;
    if (block_count < 1) {
        block_count = 1;
    }
    uint32_t block_index = 0;
    interface->progress.progress((float)block_index / (float)block_count);

    uint32_t location = 0; // what is the right offset
    uint32_t end = 0; // ?
    uint32_t flash_location = 0;
    while (location < end) {
        interface->watchdog.feed();

        uint32_t length = end - location;
        if (length > FD_BOOT_HASH_BLOCK_SIZE) {
            length = FD_BOOT_HASH_BLOCK_SIZE;
        }
        fd_boot_range_t subrange = {
            .location = location,
            .length = length,
        };
        uint8_t data[FD_BOOT_HASH_BLOCK_SIZE];
        if (!interface->update_reader.read(
            interface->update_reader.context,
            subrange.location,
            data,
            subrange.length,
            error
        )) {
            return false;
        }
        uint8_t out[FD_BOOT_HASH_BLOCK_SIZE];
        if (!interface->decrypt.update(&decrypt_context, data, out, length, error)) {
            return false;
        }
        if (!interface->executable_flasher.write(
            interface->executable_flasher.context,
            flash_location,
            out,
            length,
            error
        )) {
            return false;
        }
        flash_location += length;

        ++block_index;
        interface->progress.progress((float)block_index / (float)block_count);
    }

    if (!interface->executable_flasher.finalize(interface->executable_flasher.context, error)) {
        return false;
    }
    if (!interface->decrypt.finalize(&decrypt_context, error)) {
        return false;
    }

    return true;
}

bool fd_boot_update(
    fd_boot_update_interface_t *interface,
    fd_boot_update_result_t *result,
    fd_boot_error_t *error
) {
    memset(result, 0, sizeof(*result));

    fd_boot_get_executable_metadata_result_t executable;
    if (!fd_boot_get_executable_metadata(interface, &executable, error)) {
        return false;
    }
    fd_boot_get_update_metadata_result_t update;
    if (!fd_boot_get_update_metadata(interface, &update, error)) {
        return false;
    }

    fd_boot_update_action_t action = fd_boot_get_action(
        &interface->action, &executable, &update
    );
    switch (action) {
        case fd_boot_update_action_none: {
            result->issue = fd_boot_update_issue_firmware;
        } break;
        case fd_boot_update_action_run: {
            result->is_valid = true;
        } break;
        case fd_boot_update_action_install: {
            if (!fd_boot_install(interface, &update.metadata, error)) {
                return false;
            }
            fd_boot_executable_metadata_t executable_metadata;
            if (!fd_boot_get_executable_metadata(interface, &executable, error)) {
                return false;
            }
            if (!executable.is_valid) {
                result->issue = fd_boot_update_issue_update;
            } else {
                result->is_valid = true;
            }
        } break;
    }
    return true;
}

#if 0
void fd_boot_run_application(void) {
    SCB->VTOR = FD_BOOT_APPLICATION_START;
    uint32_t *vector_table = (uint32_t *)FD_BOOT_APPLICATION_START;
    uint32_t sp = vector_table[0];
    uint32_t pc = vector_table[1];
    __asm volatile(
        "   msr msp, %[sp]\n"
        "   msr psp, %[sp]\n"
        "   mov pc, %[pc]\n"
        :
        : [sp] "r" (sp), [pc] "r" (pc)
    );
}
#endif