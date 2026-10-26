#include <ecode.h>
#include <limits.h>
#include <path.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int isreg(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat)) { return 0; }
    return S_ISREG(path_stat.st_mode);
}

int isdir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat)) { return 0; }
    return S_ISDIR(path_stat.st_mode);
}

char *path_join(const char *a, const char *b) {
    size_t      alen   = strlen(a);
    const char *sep    = "/";
    size_t      outlen = strlen(a) + 1 + strlen(b) + 1;
    char       *out;

    if (alen == 0 || a[alen - 1] == '/') { sep = ""; }
    out = malloc(outlen);
    if (!out) { return NULL; }
    int n = snprintf(out, outlen, "%s%s%s", a, sep, b);
    if (n < 0 || (size_t) n >= outlen) {
        free(out);
        return NULL;
    }
    return out;
}

char *pfroot(const char *path) {
    size_t n    = strcspn(path, "/");
    char  *root = malloc(n + 1);
    if (!root) { return NULL; }
    memcpy(root, path, n);
    root[n] = '\0';
    return root;
}

char *pftail(const char *path) {
    size_t      n = strcspn(path, "/"), tlen;
    const char *tail;
    char       *t;
    tail = path + n;
    tail += strspn(tail, "/");

    tlen = strlen(tail);
    t    = malloc(tlen + 1);
    if (!t) { return NULL; }
    memcpy(t, tail, tlen + 1);
    return t;
}

char *proot(const char *path) {
    size_t len = strlen(path);
    size_t i, end;

    while (len > 1 && path[len - 1] == '/') { len--; }
    i = len;
    while (i > 0 && path[i - 1] != '/') { i--; }

    if (i == 0) { return strdup("."); }

    end = i;
    while (end > 1 && path[end - 1] == '/') { end--; }

    char *root = malloc(end + 1);
    if (!root) { return NULL; }
    memcpy(root, path, end);
    root[end] = '\0';
    return root;
}

char *ptail(const char *path) {
    size_t len = strlen(path);
    size_t i;

    while (len > 1 && path[len - 1] == '/') { len--; }
    i = len;
    while (i > 0 && path[i - 1] != '/') { i--; }

    size_t tlen = len - i;
    char  *tail = malloc(tlen + 1);
    if (!tail) { return NULL; }
    memcpy(tail, path + i, tlen);
    tail[tlen] = '\0';
    return tail;
}

static void normalize(char *path) {
    char       *out = path;
    const char *p   = path;

    while (*p) {
        p += strspn(p, "/");
        size_t len = strcspn(p, "/");
        if (len == 0) { break; }

        if (len == 1 && p[0] == '.') {
            /* skip */
        } else if (len == 2 && p[0] == '.' && p[1] == '.') {
            while (out > path && *--out != '/') {}
        } else {
            *out++ = '/';
            memmove(out, p, len);
            out += len;
        }
        p += len;
    }
    if (out == path) { *out++ = '/'; }
    *out = '\0';
}

char *abspath(const char *path) {
    char  *cwd = NULL, *out;
    size_t len = strlen(path);
    size_t cwdlen;

    if (path[0] != '/') {
        cwd = getcwd(NULL, 0);
        if (!cwd) { return NULL; }
    }

    cwdlen = cwd ? strlen(cwd) : 0;
    out    = malloc(cwdlen + 1 + len + 1);
    if (!out) { goto fail0; }

    if (cwd) {
        sprintf(out, "%s/%s", cwd, path);
    } else {
        strcpy(out, path);
    }
    free(cwd);

    normalize(out);
    return out;

fail0:
    free(cwd);
    return NULL;
}

#define MAXC 512
_Static_assert((int) EC_RUNTIMEERR < 0, "EC_RUNTIMEERR must be negative");

static int split(const char *p, char *buf, char **comp) {
    int   n;
    char *save;

    if (p[0] == '/') {
        snprintf(buf, 2 * PATH_MAX, "%s", p);
    } else {
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd))) { return EC_RUNTIMEERR; }
        snprintf(buf, 2 * PATH_MAX, "%s/%s", cwd, p);
    }

    n = 0;
    for (char *t = strtok_r(buf, "/", &save); t; t = strtok_r(NULL, "/", &save)) {
        if (!strcmp(t, ".")) { continue; }
        if (!strcmp(t, "..")) {
            if (n) { n--; }
            continue;
        }
        if (n < MAXC) { comp[n++] = t; }
    }

    return n;
}

static int relpath_n(const char *a, const char *b, char *out, size_t outsz) {
    char   abuf[2 * PATH_MAX], bbuf[2 * PATH_MAX];
    char  *ac[MAXC], *bc[MAXC];
    int    an, bn, i;
    size_t len;

    an = split(a, abuf, ac);
    bn = split(b, bbuf, bc);
    if (an < 0 || bn < 0) return EC_RUNTIMEERR;

    i = 0;
    while (i < an && i < bn && !strcmp(ac[i], bc[i])) { i++; }

    len    = 0;
    out[0] = '\0';

    for (int j = i; j < bn; j++) { len += snprintf(out + len, outsz - len, "%s..", len ? "/" : ""); }
    for (int j = i; j < an; j++) { len += snprintf(out + len, outsz - len, "%s%s", len ? "/" : "", ac[j]); }

    if (!len) snprintf(out, outsz, ".");
    return 0;
}

char *relpath(const char *a, const char *b) {
    char *buf = malloc(PATH_MAX), *tmp;
    if (!buf) { return NULL; }
    if (relpath_n(a, b, buf, PATH_MAX)) {
        free(buf);
        return NULL;
    }
    tmp = realloc(buf, strlen(buf) + 1);
    if (!tmp) {
        free(buf);
        return NULL;
    }
    return tmp;
}
