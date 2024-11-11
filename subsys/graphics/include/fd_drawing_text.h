#ifndef fd_drawing_text_h
#define fd_drawing_text_h

#include "fd_drawing.h"
#include "fd_view_text.h"

#include <string.h>

typedef struct {
    fd_view_text_t view;
    
    fd_view_text_t previous_view;
    fd_graphics_area_t area;
    fd_graphics_point_t origin;
    void (*update)(void *drawing_text_n);
} fd_drawing_text_t;

extern fd_drawing_class_t fd_drawing_text_class;

#define fd_drawing_text_buffer_declare(prefix, n) \
typedef struct {\
    fd_drawing_text_t base;\
    char buffer[n];\
    char previous_buffer[n];\
} prefix ## _drawing_text_ ## n ## _t;\
\
void prefix ## _drawing_text_ ## n ## _update(void *drawing_text_n);\
void prefix ## _drawing_text_ ## n ## _setup(prefix ## _drawing_text_ ## n ## _t *drawing);

#define fd_drawing_text_buffer_implement(prefix, n) \
void prefix ## _drawing_text_ ## n ## _update(void *drawing_text_n) {\
    prefix ## _drawing_text_ ## n ## _t *drawing = drawing_text_n;\
    memcpy(drawing->previous_buffer, drawing->buffer, sizeof(drawing->previous_buffer));\
}\
void prefix ## _drawing_text_ ## n ## _setup(prefix ## _drawing_text_ ## n ## _t *drawing) {\
    fd_drawing_text_t *base = &drawing->base;\
    base->view.string.string = drawing->buffer;\
    base->view.string.size = sizeof(drawing->buffer);\
    base->previous_view.string.string = drawing->previous_buffer;\
    base->previous_view.string.size = sizeof(drawing->previous_buffer);\
    base->update = prefix ## _drawing_text_ ## n ## _update;\
}

fd_drawing_text_buffer_declare(fd, 8)
fd_drawing_text_buffer_declare(fd, 16)
fd_drawing_text_buffer_declare(fd, 32)

#endif
