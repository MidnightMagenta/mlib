#ifndef LIB_PATH_H
#define LIB_PATH_H

#include <stddef.h>

int   isreg(const char *path);
int   isdir(const char *path);
char *path_join(const char *a, const char *b);
char *pfroot(const char *path);
char *pftail(const char *path);
char *proot(const char *path);
char *ptail(const char *path);
char *abspath(const char *path);

#endif
