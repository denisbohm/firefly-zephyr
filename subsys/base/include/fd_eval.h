#ifndef fd_eval_h
#define fd_eval_h

#include "fd_source.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

fd_source_push()

typedef struct {
    const char *string;
    size_t length;
} fd_eval_string_t;

typedef enum {
    fd_eval_value_type_boolean,
    fd_eval_value_type_integer,
    fd_eval_value_type_real,
} fd_eval_value_type_t;

typedef struct {
    fd_eval_value_type_t type;
    union {
        bool boolean;
        int64_t integer;
        double real;
    } ;
} fd_eval_value_t;

#define fd_eval_heap_align __attribute__ ((aligned (8)))

typedef bool (*fd_eval_get_symbol_value_t)(fd_eval_string_t symbol, fd_eval_value_t *value);

typedef struct {
    bool success;
    fd_eval_value_t value;
} fd_eval_result_t;

// return the result of evaluating the expression
fd_eval_result_t fd_eval_calculate(fd_eval_string_t expression, void *heap, size_t heap_size, fd_eval_get_symbol_value_t get_symbol_value);

// returns true for a valid expression (does not evaluate it)
bool fd_eval_check(fd_eval_string_t expression);

// return true when the two strings are equal
bool fd_eval_string_equals(fd_eval_string_t a, const char *b);

// initialize a eval string from a C string
fd_eval_string_t fd_eval_string_initialize(const char *string);

// initialize a eval value
fd_eval_value_t fd_eval_value_initialize_boolean(double boolean);
fd_eval_value_t fd_eval_value_initialize_integer(double integer);
fd_eval_value_t fd_eval_value_initialize_real(double real);

// data type conversions
fd_eval_value_t fd_eval_as_boolean(fd_eval_value_t value);
fd_eval_value_t fd_eval_as_integer(fd_eval_value_t value);
fd_eval_value_t fd_eval_as_real(fd_eval_value_t value);

fd_source_pop()

#endif