/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.4.6 */

#include "options.pb.h"
#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Definition for extension field firefly_options_v1_c_prefix */
typedef struct _firefly_options_v1_c_prefix_extmsg { 
    pb_callback_t c_prefix;
} firefly_options_v1_c_prefix_extmsg;
#define firefly_options_v1_c_prefix_extmsg_FIELDLIST(X, a) \
X(a, CALLBACK, REQUIRED, STRING,   c_prefix,        60000)
#define firefly_options_v1_c_prefix_extmsg_CALLBACK pb_default_field_callback
#define firefly_options_v1_c_prefix_extmsg_DEFAULT NULL
pb_byte_t firefly_options_v1_c_prefix_extmsg_default[] = {0x00};
PB_BIND(firefly_options_v1_c_prefix_extmsg, firefly_options_v1_c_prefix_extmsg, 4)
const pb_extension_type_t firefly_options_v1_c_prefix = {
    NULL,
    NULL,
    &firefly_options_v1_c_prefix_extmsg_msg
};

/* Definition for extension field firefly_options_v1_python_prefix */
typedef struct _firefly_options_v1_python_prefix_extmsg { 
    pb_callback_t python_prefix;
} firefly_options_v1_python_prefix_extmsg;
#define firefly_options_v1_python_prefix_extmsg_FIELDLIST(X, a) \
X(a, CALLBACK, REQUIRED, STRING,   python_prefix,   60001)
#define firefly_options_v1_python_prefix_extmsg_CALLBACK pb_default_field_callback
#define firefly_options_v1_python_prefix_extmsg_DEFAULT NULL
pb_byte_t firefly_options_v1_python_prefix_extmsg_default[] = {0x00};
PB_BIND(firefly_options_v1_python_prefix_extmsg, firefly_options_v1_python_prefix_extmsg, 4)
const pb_extension_type_t firefly_options_v1_python_prefix = {
    NULL,
    NULL,
    &firefly_options_v1_python_prefix_extmsg_msg
};

/* Definition for extension field firefly_options_v1_package_id */
typedef struct _firefly_options_v1_package_id_extmsg { 
    uint32_t package_id;
} firefly_options_v1_package_id_extmsg;
#define firefly_options_v1_package_id_extmsg_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   package_id,      60002)
#define firefly_options_v1_package_id_extmsg_CALLBACK NULL
#define firefly_options_v1_package_id_extmsg_DEFAULT NULL
pb_byte_t firefly_options_v1_package_id_extmsg_default[] = {0x00};
PB_BIND(firefly_options_v1_package_id_extmsg, firefly_options_v1_package_id_extmsg, 4)
const pb_extension_type_t firefly_options_v1_package_id = {
    NULL,
    NULL,
    &firefly_options_v1_package_id_extmsg_msg
};

/* Definition for extension field firefly_options_v1_service_id */
typedef struct _firefly_options_v1_service_id_extmsg { 
    uint32_t service_id;
} firefly_options_v1_service_id_extmsg;
#define firefly_options_v1_service_id_extmsg_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   service_id,      60003)
#define firefly_options_v1_service_id_extmsg_CALLBACK NULL
#define firefly_options_v1_service_id_extmsg_DEFAULT NULL
pb_byte_t firefly_options_v1_service_id_extmsg_default[] = {0x00};
PB_BIND(firefly_options_v1_service_id_extmsg, firefly_options_v1_service_id_extmsg, 4)
const pb_extension_type_t firefly_options_v1_service_id = {
    NULL,
    NULL,
    &firefly_options_v1_service_id_extmsg_msg
};

/* Definition for extension field firefly_options_v1_method_id */
typedef struct _firefly_options_v1_method_id_extmsg { 
    uint32_t method_id;
} firefly_options_v1_method_id_extmsg;
#define firefly_options_v1_method_id_extmsg_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   method_id,       60004)
#define firefly_options_v1_method_id_extmsg_CALLBACK NULL
#define firefly_options_v1_method_id_extmsg_DEFAULT NULL
pb_byte_t firefly_options_v1_method_id_extmsg_default[] = {0x00};
PB_BIND(firefly_options_v1_method_id_extmsg, firefly_options_v1_method_id_extmsg, 4)
const pb_extension_type_t firefly_options_v1_method_id = {
    NULL,
    NULL,
    &firefly_options_v1_method_id_extmsg_msg
};

/* Definition for extension field firefly_options_v1_access */
typedef struct _firefly_options_v1_access_extmsg { 
    uint32_t access;
} firefly_options_v1_access_extmsg;
#define firefly_options_v1_access_extmsg_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   access,          60005)
#define firefly_options_v1_access_extmsg_CALLBACK NULL
#define firefly_options_v1_access_extmsg_DEFAULT NULL
pb_byte_t firefly_options_v1_access_extmsg_default[] = {0x00};
PB_BIND(firefly_options_v1_access_extmsg, firefly_options_v1_access_extmsg, 4)
const pb_extension_type_t firefly_options_v1_access = {
    NULL,
    NULL,
    &firefly_options_v1_access_extmsg_msg
};


