/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.6 */

#ifndef PB_FIREFLY_STREAM_V1_STREAM_PB_H_INCLUDED
#define PB_FIREFLY_STREAM_V1_STREAM_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _firefly_stream_v1_Value { 
    pb_size_t which_kind;
    union {
        bool bool_value;
        uint32_t uint32_value;
        int32_t int32_value;
        float float_value;
    } kind;
} firefly_stream_v1_Value;

typedef struct _firefly_stream_v1_ConnectionConfiguration { 
    bool has_maximum_flow_window;
    firefly_stream_v1_Value maximum_flow_window;
    bool has_maximum_transmission_unit;
    firefly_stream_v1_Value maximum_transmission_unit;
    bool has_receive_keep_alive;
    firefly_stream_v1_Value receive_keep_alive;
    bool has_send_keep_alive;
    firefly_stream_v1_Value send_keep_alive;
    bool has_send_retry_timeout;
    firefly_stream_v1_Value send_retry_timeout;
} firefly_stream_v1_ConnectionConfiguration;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define firefly_stream_v1_Value_init_default     {0, {0}}
#define firefly_stream_v1_ConnectionConfiguration_init_default {false, firefly_stream_v1_Value_init_default, false, firefly_stream_v1_Value_init_default, false, firefly_stream_v1_Value_init_default, false, firefly_stream_v1_Value_init_default, false, firefly_stream_v1_Value_init_default}
#define firefly_stream_v1_Value_init_zero        {0, {0}}
#define firefly_stream_v1_ConnectionConfiguration_init_zero {false, firefly_stream_v1_Value_init_zero, false, firefly_stream_v1_Value_init_zero, false, firefly_stream_v1_Value_init_zero, false, firefly_stream_v1_Value_init_zero, false, firefly_stream_v1_Value_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define firefly_stream_v1_Value_bool_value_tag   1
#define firefly_stream_v1_Value_uint32_value_tag 2
#define firefly_stream_v1_Value_int32_value_tag  3
#define firefly_stream_v1_Value_float_value_tag  4
#define firefly_stream_v1_ConnectionConfiguration_maximum_flow_window_tag 1
#define firefly_stream_v1_ConnectionConfiguration_maximum_transmission_unit_tag 2
#define firefly_stream_v1_ConnectionConfiguration_receive_keep_alive_tag 3
#define firefly_stream_v1_ConnectionConfiguration_send_keep_alive_tag 4
#define firefly_stream_v1_ConnectionConfiguration_send_retry_timeout_tag 5

/* Struct field encoding specification for nanopb */
#define firefly_stream_v1_Value_FIELDLIST(X, a) \
X(a, STATIC,   ONEOF,    BOOL,     (kind,bool_value,kind.bool_value),   1) \
X(a, STATIC,   ONEOF,    UINT32,   (kind,uint32_value,kind.uint32_value),   2) \
X(a, STATIC,   ONEOF,    INT32,    (kind,int32_value,kind.int32_value),   3) \
X(a, STATIC,   ONEOF,    FLOAT,    (kind,float_value,kind.float_value),   4)
#define firefly_stream_v1_Value_CALLBACK NULL
#define firefly_stream_v1_Value_DEFAULT NULL

#define firefly_stream_v1_ConnectionConfiguration_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, MESSAGE,  maximum_flow_window,   1) \
X(a, STATIC,   OPTIONAL, MESSAGE,  maximum_transmission_unit,   2) \
X(a, STATIC,   OPTIONAL, MESSAGE,  receive_keep_alive,   3) \
X(a, STATIC,   OPTIONAL, MESSAGE,  send_keep_alive,   4) \
X(a, STATIC,   OPTIONAL, MESSAGE,  send_retry_timeout,   5)
#define firefly_stream_v1_ConnectionConfiguration_CALLBACK NULL
#define firefly_stream_v1_ConnectionConfiguration_DEFAULT NULL
#define firefly_stream_v1_ConnectionConfiguration_maximum_flow_window_MSGTYPE firefly_stream_v1_Value
#define firefly_stream_v1_ConnectionConfiguration_maximum_transmission_unit_MSGTYPE firefly_stream_v1_Value
#define firefly_stream_v1_ConnectionConfiguration_receive_keep_alive_MSGTYPE firefly_stream_v1_Value
#define firefly_stream_v1_ConnectionConfiguration_send_keep_alive_MSGTYPE firefly_stream_v1_Value
#define firefly_stream_v1_ConnectionConfiguration_send_retry_timeout_MSGTYPE firefly_stream_v1_Value

extern const pb_msgdesc_t firefly_stream_v1_Value_msg;
extern const pb_msgdesc_t firefly_stream_v1_ConnectionConfiguration_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define firefly_stream_v1_Value_fields &firefly_stream_v1_Value_msg
#define firefly_stream_v1_ConnectionConfiguration_fields &firefly_stream_v1_ConnectionConfiguration_msg

/* Maximum encoded size of messages (where known) */
#define firefly_stream_v1_ConnectionConfiguration_size 65
#define firefly_stream_v1_Value_size             11

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif