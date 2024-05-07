#include "fd_eval.h"

#include "fd_assert.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

fd_source_push()

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

typedef struct {
    const char *string;
    uint32_t precedence;
    fd_eval_value_t (*evaluate)(fd_eval_value_t value); 
} fd_eval_unary_operator_t;

fd_eval_value_t fd_eval_unary_plus_evaluate(fd_eval_value_t value);
fd_eval_value_t fd_eval_unary_minus_evaluate(fd_eval_value_t value);
fd_eval_value_t fd_eval_unary_not_evaluate(fd_eval_value_t value);

static const fd_eval_unary_operator_t fd_eval_unary_operators[] = {
    { .string = "+", .precedence = 7, .evaluate = fd_eval_unary_plus_evaluate },
    { .string = "-", .precedence = 7, .evaluate = fd_eval_unary_minus_evaluate },
    { .string = "!", .precedence = 7, .evaluate = fd_eval_unary_not_evaluate },
};

typedef enum {
    fd_eval_associativity_left,
    fd_eval_associativity_right,
} fd_eval_associativity_t;

typedef struct {
    const char *string;
    uint32_t precedence;
    fd_eval_associativity_t associativity;
    fd_eval_value_t (*evaluate)(fd_eval_value_t a, fd_eval_value_t b); 
} fd_eval_binary_operator_t;

fd_eval_value_t fd_eval_binary_multiply_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_divide_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_add_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_subtract_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_less_than_or_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_less_than_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_greater_than_or_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_greater_than_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_not_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_and_evaluate(fd_eval_value_t a, fd_eval_value_t b);
fd_eval_value_t fd_eval_binary_or_evaluate(fd_eval_value_t a, fd_eval_value_t b);

static const fd_eval_binary_operator_t fd_eval_binary_operators[] = {
    { .string = "*", .precedence = 6, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_multiply_evaluate },
    { .string = "/", .precedence = 6, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_divide_evaluate },
    { .string = "+", .precedence = 5, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_add_evaluate },
    { .string = "-", .precedence = 5, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_subtract_evaluate },
    { .string = "<=", .precedence = 4, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_less_than_or_equal_to_evaluate },
    { .string = "<", .precedence = 4, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_less_than_evaluate },
    { .string = ">=", .precedence = 4, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_greater_than_or_equal_to_evaluate },
    { .string = ">", .precedence = 4, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_greater_than_evaluate },
    { .string = "==", .precedence = 3, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_equal_to_evaluate },
    { .string = "!=", .precedence = 3, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_not_equal_to_evaluate },
    { .string = "&&", .precedence = 2, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_and_evaluate },
    { .string = "||", .precedence = 1, .associativity = fd_eval_associativity_left, .evaluate = fd_eval_binary_or_evaluate },
};

typedef enum {
    fd_eval_node_type_leaf,
    fd_eval_node_type_unary,
    fd_eval_node_type_binary,
} fd_eval_node_type_t;

struct fd_eval_node_s;
typedef struct fd_eval_node_s fd_eval_node_t;

struct fd_eval_node_s {
    fd_eval_node_type_t type;
    union {
        struct {
            fd_eval_string_t token;
        } leaf;
        struct {
            const fd_eval_unary_operator_t *operator;
            fd_eval_node_t *node;
        } unary;
        struct {
            const fd_eval_binary_operator_t *operator;
            fd_eval_node_t *left;
            fd_eval_node_t *right;
        } binary;
    };
};

typedef struct {
    fd_eval_node_t *nodes;
    uint32_t count;
    uint32_t index;
} fd_eval_node_heap_t;

typedef struct {
    bool (*get_symbol_value)(fd_eval_string_t token, fd_eval_value_t *value);
    fd_eval_string_t input;
    size_t input_index;
    fd_eval_node_heap_t heap;
    fd_eval_node_t error_node;
    fd_eval_value_t error_value;
    bool error;
    fd_eval_node_t *tree;
} fd_eval_t;

fd_eval_string_t fd_eval_string_initialize(const char *string) {
    return (fd_eval_string_t) { .string = string, .length = strlen(string) };
}

fd_eval_value_t fd_eval_value_initialize_boolean(double boolean) {
    return (fd_eval_value_t) { .type = fd_eval_value_type_boolean, .boolean = boolean };
}

fd_eval_value_t fd_eval_value_initialize_integer(double integer) {
    return (fd_eval_value_t) { .type = fd_eval_value_type_integer, .integer = integer };
}

fd_eval_value_t fd_eval_value_initialize_real(double real) {
    return (fd_eval_value_t) { .type = fd_eval_value_type_real, .real = real };
}

fd_eval_node_t *fd_eval_allocate_node(fd_eval_t *eval) {
    fd_eval_node_heap_t *heap = &eval->heap;
    if (heap->index >= heap->count) {
        // heap count of zero is a special case used for parsing with no heap, but not calculating -denis
        if (heap->count > 0) {
            eval->error = true;
        }
        return &eval->error_node;
    }
    return &heap->nodes[heap->index++];
}

fd_eval_node_t *fd_eval_allocate_leaf(fd_eval_t *eval, fd_eval_string_t token) {
    fd_eval_node_t *leaf = fd_eval_allocate_node(eval);
    leaf->type = fd_eval_node_type_leaf;
    leaf->leaf.token = token;
    return leaf;
}

fd_eval_node_t *fd_eval_allocate_unary(fd_eval_t *eval, const fd_eval_unary_operator_t *operator, fd_eval_node_t *node) {
    fd_eval_node_t *unary = fd_eval_allocate_node(eval);
    unary->type = fd_eval_node_type_unary;
    unary->unary.operator = operator;
    unary->unary.node = node;
    return unary;
}

fd_eval_node_t *fd_eval_allocate_binary(fd_eval_t *eval, const fd_eval_binary_operator_t *operator, fd_eval_node_t *left, fd_eval_node_t *right) {
    fd_eval_node_t *binary = fd_eval_allocate_node(eval);
    binary->type = fd_eval_node_type_binary;
    binary->binary.operator = operator;
    binary->binary.left = left;
    binary->binary.right = right;
    return binary;
}

bool fd_eval_is_white_space(char c) {
    return c == ' ';
}

bool fd_eval_is_alphabetic(char c) {
    return ('a' <= c) && (c <= 'z');
}

bool fd_eval_is_digit(char c) {
    return ('0' <= c) && (c <= '9');
}

bool fd_eval_is_identifier(char c) {
    return fd_eval_is_alphabetic(c) || fd_eval_is_digit(c) || (c == '_');
}

bool fd_eval_is_value(fd_eval_string_t token) {
    if (token.length <= 0) {
        return false;
    }
    char c = token.string[0];
    return fd_eval_is_alphabetic(c) || fd_eval_is_digit(c);
}

fd_eval_string_t fd_eval_next(fd_eval_t *eval) {
    size_t start = eval->input_index;

    // skip white space
    while ((start < eval->input.length) && fd_eval_is_white_space(eval->input.string[start])) {
        ++start;
    }

    if (start >= eval->input.length) {
        fd_eval_string_t end = { .string = 0, .length = 0 };
        return end;
    }

    for (uint32_t i = 0; i < ARRAY_SIZE(fd_eval_binary_operators); ++i) {
        const fd_eval_binary_operator_t *operator = &fd_eval_binary_operators[i];
        size_t operator_string_length = strlen(operator->string);
        if ((start + operator_string_length) <= eval->input.length) {
            if (memcmp(&eval->input.string[start], operator->string, operator_string_length) == 0) {
                fd_eval_string_t token = { .string = &eval->input.string[start], .length = operator_string_length };
                return token;
            }
        }
    }

    for (uint32_t i = 0; i < ARRAY_SIZE(fd_eval_unary_operators); ++i) {
        const fd_eval_unary_operator_t *operator = &fd_eval_unary_operators[i];
        size_t operator_string_length = strlen(operator->string);
        if ((start + operator_string_length) <= eval->input.length) {
            if (memcmp(&eval->input.string[start], operator->string, operator_string_length) == 0) {
                fd_eval_string_t token = { .string = &eval->input.string[start], .length = operator_string_length };
                return token;
            }
        }
    }

    char c = eval->input.string[start];

    if (fd_eval_is_digit(c)) {
        size_t index = start + 1;
        while ((index < eval->input.length) && fd_eval_is_digit(eval->input.string[index])) {
            ++index;
        }
        if ((index < eval->input.length) && (eval->input.string[index] == '.')) {
            ++index;
            while ((index < eval->input.length) && fd_eval_is_digit(eval->input.string[index])) {
                ++index;
            }
        }
        fd_eval_string_t token = { .string = &eval->input.string[start], .length = index - start };
        return token;
    }

    if (fd_eval_is_alphabetic(c)) {
        size_t index = start + 1;
        while ((index < eval->input.length) && fd_eval_is_identifier(eval->input.string[index])) {
            ++index;
        }
        fd_eval_string_t token = { .string = &eval->input.string[start], .length = index - start };
        return token;
    }

    if ((c == '(') || (c == ')')) {
        fd_eval_string_t token = { .string = &eval->input.string[start], .length = 1 };
        return token;
    }

    fd_eval_string_t error = { .string = &eval->input.string[start], .length = 0 };
    return error;
}

void fd_eval_consume(fd_eval_t *eval) {
    fd_eval_string_t token = fd_eval_next(eval);
    eval->input_index = (token.string + token.length) - eval->input.string;
}

bool fd_eval_string_equals_string(fd_eval_string_t a, fd_eval_string_t b) {
    return (a.length == b.length) && (memcmp(a.string, b.string, b.length) == 0);
}

bool fd_eval_string_equals(fd_eval_string_t a, const char *b) {
    fd_eval_string_t b_string = { .string = b, .length = strlen(b) };
    return fd_eval_string_equals_string(a, b_string);
}

void fd_eval_expect(fd_eval_t *eval, fd_eval_string_t string) {
    fd_eval_string_t token = fd_eval_next(eval);
    if (fd_eval_string_equals_string(token, string)) {
        fd_eval_consume(eval);
    } else {
        eval->error = true;
    }
}

const fd_eval_unary_operator_t *fd_eval_is_unary(fd_eval_string_t string) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_eval_unary_operators); ++i) {
        const fd_eval_unary_operator_t *operator = &fd_eval_unary_operators[i];
        if ((strlen(operator->string) == string.length) && (memcmp(operator->string, string.string, string.length) == 0)) {
            return operator;
        }
    }
    return NULL;
}

const fd_eval_binary_operator_t *fd_eval_is_binary(fd_eval_string_t string) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_eval_binary_operators); ++i) {
        const fd_eval_binary_operator_t *operator = &fd_eval_binary_operators[i];
        if ((strlen(operator->string) == string.length) && (memcmp(operator->string, string.string, string.length) == 0)) {
            return operator;
        }
    }
    return NULL;
}

fd_eval_node_t *fd_eval_expression(fd_eval_t *eval, uint32_t precedence);

fd_eval_node_t *fd_eval_highest_priority(fd_eval_t *eval) {
    fd_eval_string_t next = fd_eval_next(eval);
    const fd_eval_unary_operator_t *unary = fd_eval_is_unary(next);
    if (unary != NULL) {
        fd_eval_consume(eval);
        fd_eval_node_t *node = fd_eval_expression(eval, unary->precedence);
        return fd_eval_allocate_unary(eval, unary, node);
    }
    static const fd_eval_string_t open_paranthesis = { .string = "(", .length = 1 };
    static const fd_eval_string_t close_paranthesis = { .string = ")", .length = 1 };
    if (fd_eval_string_equals_string(next, open_paranthesis)) {
        fd_eval_consume(eval);
        fd_eval_node_t *node = fd_eval_expression(eval, 0);
        fd_eval_expect(eval, close_paranthesis);
        return node;
    }
    if (fd_eval_is_value(next)) {
        fd_eval_consume(eval);
        return fd_eval_allocate_leaf(eval, next);
    }
    eval->error = true;
    return &eval->error_node;
}

fd_eval_node_t *fd_eval_expression(fd_eval_t *eval, uint32_t precedence) {
    fd_eval_node_t *node = fd_eval_highest_priority(eval);
    while (true) {
        fd_eval_string_t next = fd_eval_next(eval);
        const fd_eval_binary_operator_t *binary = fd_eval_is_binary(next);
        if ((binary == NULL) || (binary->precedence < precedence)) {
            break;
        }
        fd_eval_consume(eval);
        uint32_t next_precedence = binary->associativity == fd_eval_associativity_right ? binary->precedence : 1 + binary->precedence;
        fd_eval_node_t *right = fd_eval_expression(eval, next_precedence);
        node = fd_eval_allocate_binary(eval, binary, node, right);
    }
    return node;
}

bool fd_eval_get_symbol_value_default(fd_eval_string_t token, fd_eval_value_t *value) {
    return false;
}

void fd_eval_parse(fd_eval_t *eval, fd_eval_string_t expression, void *heap, size_t heap_size, fd_eval_get_symbol_value_t get_symbol_value) {
    eval->input = expression;
    eval->heap.nodes = heap;
    eval->heap.count = heap_size / sizeof(fd_eval_node_t);
    eval->get_symbol_value = get_symbol_value != NULL ? get_symbol_value : fd_eval_get_symbol_value_default;

    eval->tree = fd_eval_expression(eval, 0);
    fd_eval_string_t end = {};
    fd_eval_expect(eval, end);
}

bool fd_eval_check(fd_eval_string_t expression) {
    fd_eval_t eval = {};
    fd_eval_parse(&eval, expression, NULL, 0, NULL);
    return !eval.error;
}

fd_eval_value_t fd_eval_unary_plus_evaluate(fd_eval_value_t value) {
    return value;
}

fd_eval_value_t fd_eval_unary_minus_evaluate(fd_eval_value_t value) {
    switch (value.type) {
        case fd_eval_value_type_boolean: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_boolean,
                .boolean = !value.boolean
            };
            return result;
        } break;
        case fd_eval_value_type_integer: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_integer,
                .integer = -value.integer
            };
            return result;
        } break;
        case fd_eval_value_type_real: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_real,
                .real = -value.real
            };
            return result;
        } break;
    }
    return value;
}

fd_eval_value_t fd_eval_unary_not_evaluate(fd_eval_value_t value) {
    switch (value.type) {
        case fd_eval_value_type_boolean: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_boolean,
                .boolean = !value.boolean
            };
            return result;
        } break;
        case fd_eval_value_type_integer: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_boolean,
                .integer = value.integer == 0
            };
            return result;
        } break;
        case fd_eval_value_type_real: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_boolean,
                .real = value.real == 0.0
            };
            return result;
        } break;
    }
    return value;
}

fd_eval_value_t fd_eval_as_real(fd_eval_value_t value) {
    switch (value.type) {
        case fd_eval_value_type_boolean: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_real,
                .real = value.boolean ? 1.0 : 0.0
            };
            return result;
        } break;
        case fd_eval_value_type_integer: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_real,
                .real = (double)value.integer
            };
            return result;
        } break;
        case fd_eval_value_type_real:
            return value;
    }
    return value;
}

fd_eval_value_t fd_eval_as_integer(fd_eval_value_t value) {
    switch (value.type) {
        case fd_eval_value_type_boolean: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_integer,
                .integer = value.boolean ? 1 : 0
            };
            return result;
        } break;
        case fd_eval_value_type_integer:
            return value;
        case fd_eval_value_type_real: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_integer,
                .real = round(value.real)
            };
            return result;
        } break;
    }
    return value;
}

fd_eval_value_t fd_eval_as_boolean(fd_eval_value_t value) {
    switch (value.type) {
        case fd_eval_value_type_boolean:
            return value;
        case fd_eval_value_type_integer: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_boolean,
                .boolean = value.integer != 0
            };
            return result;
        } break;
        case fd_eval_value_type_real: {
            fd_eval_value_t result = {
                .type = fd_eval_value_type_boolean,
                .boolean = value.integer != 0.0
            };
            return result;
        } break;
    }
    return value;
}

typedef struct {
    double (*real)(double a, double b);
    int64_t (*integer)(int64_t a, int64_t b);
    bool (*boolean)(bool a, bool b);
} fd_eval_binary_operation_t;

fd_eval_value_t fd_eval_binary_evaluate(fd_eval_value_t a, fd_eval_value_t b, fd_eval_binary_operation_t operation) {
    if ((a.type == fd_eval_value_type_real) || (b.type == fd_eval_value_type_real)) {
        fd_eval_value_t a_real = fd_eval_as_real(a);
        fd_eval_value_t b_real = fd_eval_as_real(b);
        fd_eval_value_t result = {
            .type = fd_eval_value_type_real,
            .real = operation.real(a_real.real, b_real.real)
        };
        return result;
    }
    if ((a.type == fd_eval_value_type_integer) || (b.type == fd_eval_value_type_integer)) {
        fd_eval_value_t a_integer = fd_eval_as_integer(a);
        fd_eval_value_t b_integer = fd_eval_as_integer(b);
        fd_eval_value_t result = {
            .type = fd_eval_value_type_integer,
            .integer = operation.integer(a_integer.integer, b_integer.integer)
        };
        return result;
    }
    fd_eval_value_t result = {
        .type = fd_eval_value_type_boolean,
        .boolean = operation.boolean(a.boolean, b.boolean)
    };
    return result;
}

double fd_eval_binary_multiply_evaluate_real(double a, double b) {
    return a * b;
}

int64_t fd_eval_binary_multiply_evaluate_integer(int64_t a, int64_t b) {
    return a * b;
}

bool fd_eval_binary_multiply_evaluate_boolean(bool a, bool b) {
    return a && b;
}

fd_eval_value_t fd_eval_binary_multiply_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_evaluate(a, b, (fd_eval_binary_operation_t) {
        .real = fd_eval_binary_multiply_evaluate_real,
        .integer = fd_eval_binary_multiply_evaluate_integer,
        .boolean = fd_eval_binary_multiply_evaluate_boolean,
    });
}

double fd_eval_binary_divide_evaluate_real(double a, double b) {
    return b != 0.0 ? a / b : 0.0;
}

int64_t fd_eval_binary_divide_evaluate_integer(int64_t a, int64_t b) {
    return b != 0 ? a / b : 0;
}

bool fd_eval_binary_divide_evaluate_boolean(bool a, bool b) {
    return a || b;
}

fd_eval_value_t fd_eval_binary_divide_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_evaluate(a, b, (fd_eval_binary_operation_t) {
        .real = fd_eval_binary_divide_evaluate_real,
        .integer = fd_eval_binary_divide_evaluate_integer,
        .boolean = fd_eval_binary_divide_evaluate_boolean,
    });
}

double fd_eval_binary_add_evaluate_real(double a, double b) {
    return a + b;
}

int64_t fd_eval_binary_add_evaluate_integer(int64_t a, int64_t b) {
    return a + b;
}

bool fd_eval_binary_add_evaluate_boolean(bool a, bool b) {
    return a || b;
}

fd_eval_value_t fd_eval_binary_add_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_evaluate(a, b, (fd_eval_binary_operation_t) {
        .real = fd_eval_binary_add_evaluate_real,
        .integer = fd_eval_binary_add_evaluate_integer,
        .boolean = fd_eval_binary_add_evaluate_boolean,
    });
}

double fd_eval_binary_subtract_evaluate_real(double a, double b) {
    return a - b;
}

int64_t fd_eval_binary_subtract_evaluate_integer(int64_t a, int64_t b) {
    return a - b;
}

bool fd_eval_binary_subtract_evaluate_boolean(bool a, bool b) {
    return a && b;
}

fd_eval_value_t fd_eval_binary_subtract_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_evaluate(a, b, (fd_eval_binary_operation_t) {
        .real = fd_eval_binary_subtract_evaluate_real,
        .integer = fd_eval_binary_subtract_evaluate_integer,
        .boolean = fd_eval_binary_subtract_evaluate_boolean,
    });
}

typedef struct {
    bool (*real)(double a, double b);
    bool (*integer)(int64_t a, int64_t b);
    bool (*boolean)(bool a, bool b);
} fd_eval_binary_boolean_operation_t;

fd_eval_value_t fd_eval_binary_boolean_evaluate(fd_eval_value_t a, fd_eval_value_t b, fd_eval_binary_boolean_operation_t operation) {
    if ((a.type == fd_eval_value_type_real) || (b.type == fd_eval_value_type_real)) {
        fd_eval_value_t a_real = fd_eval_as_real(a);
        fd_eval_value_t b_real = fd_eval_as_real(b);
        fd_eval_value_t result = {
            .type = fd_eval_value_type_boolean,
            .boolean = operation.real(a_real.real, b_real.real)
        };
        return result;
    }
    if ((a.type == fd_eval_value_type_integer) || (b.type == fd_eval_value_type_integer)) {
        fd_eval_value_t a_integer = fd_eval_as_integer(a);
        fd_eval_value_t b_integer = fd_eval_as_integer(b);
        fd_eval_value_t result = {
            .type = fd_eval_value_type_boolean,
            .boolean = operation.integer(a_integer.integer, b_integer.integer)
        };
        return result;
    }
    fd_eval_value_t result = {
        .type = fd_eval_value_type_boolean,
        .boolean = operation.boolean(a.boolean, b.boolean)
    };
    return result;
}

bool fd_eval_binary_less_than_or_equal_to_evaluate_real(double a, double b) {
    return a <= b;
}

bool fd_eval_binary_less_than_or_equal_to_evaluate_integer(int64_t a, int64_t b) {
    return a <= b;
}

bool fd_eval_binary_less_than_or_equal_to_evaluate_boolean(bool a, bool b) {
    return !a || b;
}

fd_eval_value_t fd_eval_binary_less_than_or_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_evaluate(a, b, (fd_eval_binary_boolean_operation_t) {
        .real = fd_eval_binary_less_than_or_equal_to_evaluate_real,
        .integer = fd_eval_binary_less_than_or_equal_to_evaluate_integer,
        .boolean = fd_eval_binary_less_than_or_equal_to_evaluate_boolean,
    });
}

bool fd_eval_binary_less_than_evaluate_real(double a, double b) {
    return a < b;
}

bool fd_eval_binary_less_than_evaluate_integer(int64_t a, int64_t b) {
    return a < b;
}

bool fd_eval_binary_less_than_evaluate_boolean(bool a, bool b) {
    return !a && b;
}

fd_eval_value_t fd_eval_binary_less_than_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_evaluate(a, b, (fd_eval_binary_boolean_operation_t) {
        .real = fd_eval_binary_less_than_evaluate_real,
        .integer = fd_eval_binary_less_than_evaluate_integer,
        .boolean = fd_eval_binary_less_than_evaluate_boolean,
    });
}

bool fd_eval_binary_greater_than_or_equal_to_evaluate_real(double a, double b) {
    return a >= b;
}

bool fd_eval_binary_greater_than_or_equal_to_evaluate_integer(int64_t a, int64_t b) {
    return a >= b;
}

bool fd_eval_binary_greater_than_or_equal_to_evaluate_boolean(bool a, bool b) {
    return a || !b;
}

fd_eval_value_t fd_eval_binary_greater_than_or_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_evaluate(a, b, (fd_eval_binary_boolean_operation_t) {
        .real = fd_eval_binary_greater_than_or_equal_to_evaluate_real,
        .integer = fd_eval_binary_greater_than_or_equal_to_evaluate_integer,
        .boolean = fd_eval_binary_greater_than_or_equal_to_evaluate_boolean,
    });
}

bool fd_eval_binary_greater_than_evaluate_real(double a, double b) {
    return a > b;
}

bool fd_eval_binary_greater_than_evaluate_integer(int64_t a, int64_t b) {
    return a > b;
}

bool fd_eval_binary_greater_than_evaluate_boolean(bool a, bool b) {
    return a && !b;
}

fd_eval_value_t fd_eval_binary_greater_than_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_evaluate(a, b, (fd_eval_binary_boolean_operation_t) {
        .real = fd_eval_binary_greater_than_evaluate_real,
        .integer = fd_eval_binary_greater_than_evaluate_integer,
        .boolean = fd_eval_binary_greater_than_evaluate_boolean,
    });
}

bool fd_eval_binary_equal_to_evaluate_real(double a, double b) {
    return a == b;
}

bool fd_eval_binary_equal_to_evaluate_integer(int64_t a, int64_t b) {
    return a == b;
}

bool fd_eval_binary_equal_to_evaluate_boolean(bool a, bool b) {
    return a == b;
}

fd_eval_value_t fd_eval_binary_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_evaluate(a, b, (fd_eval_binary_boolean_operation_t) {
        .real = fd_eval_binary_equal_to_evaluate_real,
        .integer = fd_eval_binary_equal_to_evaluate_integer,
        .boolean = fd_eval_binary_equal_to_evaluate_boolean,
    });
}

bool fd_eval_binary_not_equal_to_evaluate_real(double a, double b) {
    return a != b;
}

bool fd_eval_binary_not_equal_to_evaluate_integer(int64_t a, int64_t b) {
    return a != b;
}

bool fd_eval_binary_not_equal_to_evaluate_boolean(bool a, bool b) {
    return a != b;
}

fd_eval_value_t fd_eval_binary_not_equal_to_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_evaluate(a, b, (fd_eval_binary_boolean_operation_t) {
        .real = fd_eval_binary_not_equal_to_evaluate_real,
        .integer = fd_eval_binary_not_equal_to_evaluate_integer,
        .boolean = fd_eval_binary_not_equal_to_evaluate_boolean,
    });
}

typedef struct {
   bool (*boolean)(bool a, bool b);
} fd_eval_binary_boolean_demote_operation_t;

fd_eval_value_t fd_eval_binary_boolean_demote_evaluate(fd_eval_value_t a, fd_eval_value_t b, fd_eval_binary_boolean_demote_operation_t operation) {
    if (a.type != fd_eval_value_type_boolean) {
        a = fd_eval_as_boolean(a);
    }
    if (b.type != fd_eval_value_type_boolean) {
        b = fd_eval_as_boolean(b);
    }
    fd_eval_value_t result = {
        .type = fd_eval_value_type_boolean,
        .boolean = operation.boolean(a.boolean, b.boolean)
    };
    return result;
}

bool fd_eval_binary_and_evaluate_boolean(bool a, bool b) {
    return a && b;
}

fd_eval_value_t fd_eval_binary_and_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_demote_evaluate(a, b, (fd_eval_binary_boolean_demote_operation_t) {
        .boolean = fd_eval_binary_and_evaluate_boolean,
    });
}

bool fd_eval_binary_or_evaluate_boolean(bool a, bool b) {
    return a || b;
}

fd_eval_value_t fd_eval_binary_or_evaluate(fd_eval_value_t a, fd_eval_value_t b) {
    return fd_eval_binary_boolean_demote_evaluate(a, b, (fd_eval_binary_boolean_demote_operation_t) {
        .boolean = fd_eval_binary_or_evaluate_boolean,
    });
}

fd_eval_value_t fd_eval_parse_integer(fd_eval_t *eval, fd_eval_string_t token) {
    char *end = NULL;
    int64_t integer = strtoll(token.string, &end, 10);
    fd_assert(end == (token.string + token.length));
    return (fd_eval_value_t) { .type = fd_eval_value_type_integer, .integer = integer };
}

fd_eval_value_t fd_eval_parse_real(fd_eval_t *eval, fd_eval_string_t token) {
    char *end = NULL;
    double real = strtod(token.string, &end);
    fd_assert(end == (token.string + token.length));
    return (fd_eval_value_t) { .type = fd_eval_value_type_real, .real = real };
}

fd_eval_value_t fd_eval_get_symbol_value(fd_eval_t *eval, fd_eval_string_t token) {
    fd_eval_value_t value = { .type = fd_eval_value_type_integer, .integer = 0 };
    if (eval->get_symbol_value(token, &value)) {
        return value;
    }
    eval->error = true;
    return eval->error_value;
}

fd_eval_value_t fd_eval_evaluate_leaf(fd_eval_t *eval, fd_eval_node_t *node) {
    fd_eval_string_t token = node->leaf.token;
    if (token.length == 0) {
        return eval->error_value;
    }

    static const fd_eval_string_t true_token = { .string = "true", .length = 4 };
    if (fd_eval_string_equals_string(token, true_token)) {
        return (fd_eval_value_t) { .type = fd_eval_value_type_boolean, .boolean = true };
    }
    static const fd_eval_string_t false_token = { .string = "false", .length = 5 };
    if (fd_eval_string_equals_string(token, false_token)) {
        return (fd_eval_value_t) { .type = fd_eval_value_type_boolean, .boolean = true };
    }

    char c = token.string[0];
    if (fd_eval_is_digit(c)) {
        void *decimal_point = memchr(token.string, '.', token.length);
        if (decimal_point == NULL) {
            return fd_eval_parse_integer(eval, token);
        } else {
            return fd_eval_parse_real(eval, token);
        }
    }

    return fd_eval_get_symbol_value(eval, token);
}

fd_eval_value_t fd_eval_evaluate(fd_eval_t *eval, fd_eval_node_t *node);

fd_eval_value_t fd_eval_evaluate_unary(fd_eval_t *eval, fd_eval_node_t *unary) {
    fd_eval_value_t value = fd_eval_evaluate(eval, unary->unary.node);
    return unary->unary.operator->evaluate(value);
}

fd_eval_value_t fd_eval_evaluate_binary(fd_eval_t *eval, fd_eval_node_t *binary) {
    fd_eval_value_t a = fd_eval_evaluate(eval, binary->binary.left);
    fd_eval_value_t b = fd_eval_evaluate(eval, binary->binary.right);
    return binary->binary.operator->evaluate(a, b);
}

fd_eval_value_t fd_eval_evaluate(fd_eval_t *eval, fd_eval_node_t *node) {
    switch (node->type) {
        case fd_eval_node_type_leaf:
            return fd_eval_evaluate_leaf(eval, node);
        case fd_eval_node_type_unary:
            return fd_eval_evaluate_unary(eval, node);
        case fd_eval_node_type_binary:
            return fd_eval_evaluate_binary(eval, node);
        default:
            break;
    }
    eval->error = true;
    return eval->error_value;
}

fd_eval_result_t fd_eval_calculate(fd_eval_string_t expression, void *heap, size_t heap_size, fd_eval_get_symbol_value_t get_symbol_value) {
    fd_eval_t eval = {};
    fd_eval_parse(&eval, expression, heap, heap_size, get_symbol_value);
    fd_eval_result_t result;
    result.value = fd_eval_evaluate(&eval, eval.tree);
    result.success = !eval.error;
    return result;
}

bool fd_eval_test_get_symbol_value(fd_eval_string_t symbol, fd_eval_value_t *value) {
    return false;
}

void fd_eval_test_calculate(const char *input, float expected_value) {
    fd_eval_string_t expression = {
        .string = input,
        .length = strlen(input),
    };
    uint8_t heap[256] fd_eval_heap_align;
    fd_eval_result_t result = fd_eval_calculate(expression, heap, sizeof(heap), fd_eval_test_get_symbol_value);
    fd_assert(result.success);
    if (!result.success) {
        return;
    }
    fd_eval_value_t value = fd_eval_as_real(result.value);
    const float epsilon = 0.0001;
    fd_assert(fabs(value.real - expected_value) < epsilon);
}

void fd_eval_test(void) {
    fd_eval_test_calculate("0.3 + 0.1 * 2.0", 0.5f);
    fd_eval_test_calculate("0.1 * 2.0 + 0.3", 0.5f);
}

fd_source_pop()
