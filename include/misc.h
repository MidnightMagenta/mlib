#ifndef MLIB_MISC_H
#define MLIB_MISC_H

#define ARRAY_SIZE(arr)                 (sizeof(arr) / sizeof(*arr))
#define IS_POWER_OF_TWO(x)              ((x) && !((x) & ((x) - 1)))
#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))

#endif
