#ifndef fd_source_h
#define fd_source_h

#if defined(CONFIG_FIREFLY_DEBUG)

#define fd_source_push()\
 _Pragma ("GCC push_options")\
 _Pragma ("GCC optimize (\"O0\")")\
 _Pragma ("GCC diagnostic push")\
 _Pragma ("GCC diagnostic warning \"-Wall\"")\
 _Pragma ("GCC diagnostic warning \"-Wextra\"")\
 _Pragma ("GCC diagnostic warning \"-Wstrict-prototypes\"")\
 _Pragma ("GCC diagnostic warning \"-Wno-unused-parameter\"")\
 _Pragma ("GCC diagnostic warning \"-Wunused-function\"")\
 _Pragma ("GCC diagnostic warning \"-Wignored-qualifiers\"")\
 _Pragma ("GCC diagnostic warning \"-Wmissing-field-initializers\"")\
 _Pragma ("GCC diagnostic warning \"-Wdouble-promotion\"")\
 _Pragma ("GCC diagnostic warning \"-Wabsolute-value\"")\
 _Pragma ("GCC diagnostic warning \"-Wno-switch\"")\
 _Pragma ("GCC diagnostic warning \"-Wconversion\"")\
 _Pragma ("GCC diagnostic warning \"-Wunused-parameter\"")

#else

#define fd_source_push()\
 _Pragma ("GCC push_options")\
 _Pragma ("GCC diagnostic push")

#endif

#define fd_source_push_debug()\
 fd_source_push()\
 _Pragma ("GCC optimize (\"O0\")")

#define fd_source_push_speed()\
 fd_source_push()\
 _Pragma ("GCC optimize (\"O3\")")

#define fd_source_push_size()\
 fd_source_push()\
 _Pragma ("GCC optimize (\"Oz\")")

#define fd_source_pop()\
 _Pragma ("GCC pop_options")\
 _Pragma ("GCC diagnostic pop")

#endif