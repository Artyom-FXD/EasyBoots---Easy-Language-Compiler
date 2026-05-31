/**
 * file_native.c — Native C implementation for the File module.
 *
 * Provides low-level file I/O, path operations, and metadata queries.
 * These functions are called from Ely via the link.ely extern declarations
 * and are compiled into the static library (file.lib).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#else
#include <unistd.h>
#include <limits.h>
#endif

/* ---------------- File opening / closing ---------------- */

typedef struct {
    FILE* handle;
    char* path;
    char* mode;
} ElyFile;

ElyFile* ely_file_open(const char* path, const char* mode) {
    FILE* f = fopen(path, mode);
    if (!f) return NULL;

    ElyFile* ef = (ElyFile*)malloc(sizeof(ElyFile));
    ef->handle = f;
    ef->path = strdup(path);
    ef->mode = strdup(mode);
    return ef;
}

void ely_file_close(ElyFile* f) {
    if (!f) return;
    if (f->handle) fclose(f->handle);
    free(f->path);
    free(f->mode);
    free(f);
}

/* ---------------- Read / Write ---------------- */

char* ely_file_read(ElyFile* f, size_t* out_len) {
    if (!f || !f->handle) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    fseek(f->handle, 0, SEEK_END);
    long sz = ftell(f->handle);
    rewind(f->handle);
    if (sz < 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    size_t len = (size_t)sz;
    char* buf = (char*)malloc(len + 1);
    if (!buf) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    size_t read = fread(buf, 1, len, f->handle);
    buf[read] = '\0';
    if (out_len) *out_len = read;
    return buf;
}

int ely_file_write(ElyFile* f, const char* data, size_t len) {
    if (!f || !f->handle || !data) return -1;
    size_t written = fwrite(data, 1, len, f->handle);
    return (int)written;
}

char* ely_file_read_all(const char* path, size_t* out_len) {
    ElyFile* f = ely_file_open(path, "rb");
    if (!f) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    char* data = ely_file_read(f, out_len);
    ely_file_close(f);
    return data;
}

char* ely_file_read_all_simple(const char* path) {
    return ely_file_read_all(path, NULL);
}

int ely_file_write_all(const char* path, const char* data, size_t len) {
    ElyFile* f = ely_file_open(path, "wb");
    if (!f) return -1;
    int w = ely_file_write(f, data, len);
    ely_file_close(f);
    return w;
}

int ely_file_write_all_simple(const char* path, const char* data) {
    return ely_file_write_all(path, data, strlen(data));
}

/* ---------------- Existence / Metadata ---------------- */

int ely_file_exists(const char* path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) ? 1 : 0;
#else
    struct stat st;
    return (stat(path, &st) == 0) ? 1 : 0;
#endif
}

int ely_file_remove(const char* path) {
    return remove(path);
}

int ely_file_rename(const char* old_path, const char* new_path) {
    return rename(old_path, new_path);
}

long long ely_file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (long long)st.st_size;
}

int ely_file_is_dir(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

int ely_file_is_file(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode) ? 1 : 0;
}

int ely_file_mkdir(const char* path) {
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

int ely_file_rmdir(const char* path) {
#ifdef _WIN32
    return _rmdir(path);
#else
    return rmdir(path);
#endif
}

/* ---------------- Path utilities ---------------- */

char* ely_path_join(const char* a, const char* b) {
    size_t la = strlen(a);
    int need_sep = (la > 0 && a[la - 1] != '/' && a[la - 1] != '\\') ? 1 : 0;
    size_t lb = strlen(b);
    char* res = (char*)malloc(la + lb + need_sep + 1);
    if (!res) return NULL;
    memcpy(res, a, la);
    if (need_sep) res[la] = '/';
    memcpy(res + la + need_sep, b, lb + 1);
    return res;
}

char* ely_path_basename(const char* path) {
#ifdef _WIN32
    const char* p = strrchr(path, '\\');
    if (!p) p = strrchr(path, '/');
#else
    const char* p = strrchr(path, '/');
#endif
    return p ? strdup(p + 1) : strdup(path);
}

char* ely_path_dirname(const char* path) {
    int len = (int)strlen(path);
    if (len == 0) return strdup(".");
#ifdef _WIN32
    const char* p = strrchr(path, '\\');
    if (!p) p = strrchr(path, '/');
#else
    const char* p = strrchr(path, '/');
#endif
    if (!p) return strdup(".");
    int dirlen = (int)(p - path);
    if (dirlen == 0) {
        char* r = (char*)malloc(2);
        r[0] = (p == path ? '/' : '.');
        r[1] = '\0';
        return r;
    }
    char* r = (char*)malloc(dirlen + 1);
    memcpy(r, path, dirlen);
    r[dirlen] = '\0';
    return r;
}

int ely_path_is_absolute(const char* path) {
#ifdef _WIN32
    return (path[0] && path[1] == ':') ||
           (path[0] == '\\' && path[1] == '\\');
#else
    return path[0] == '/';
#endif
}

/* ---------------- Directory listing ---------------- */

#ifdef _WIN32
#include <windows.h>

char* ely_file_list_dir(const char* path, int* count) {
    char pattern[PATH_MAX];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (count) *count = 0;
        return NULL;
    }

    /* First pass: count entries */
    int n = 0;
    do {
        if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0)
            n++;
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    if (n == 0) {
        if (count) *count = 0;
        return NULL;
    }

    /* Second pass: allocate and fill */
    char** names = (char**)malloc(n * sizeof(char*));
    hFind = FindFirstFileA(pattern, &fd);
    int i = 0;
    do {
        if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
            names[i++] = strdup(fd.cFileName);
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    /* Flatten into a single buffer: count + offsets + strings */
    size_t total = sizeof(int) + n * sizeof(char*);
    for (i = 0; i < n; i++)
        total += strlen(names[i]) + 1;

    char* buf = (char*)malloc(total);
    *(int*)buf = n;
    char** ptrs = (char**)(buf + sizeof(int));
    char* strs = buf + sizeof(int) + n * sizeof(char*);
    for (i = 0; i < n; i++) {
        strcpy(strs, names[i]);
        ptrs[i] = strs;
        strs += strlen(names[i]) + 1;
        free(names[i]);
    }
    free(names);
    if (count) *count = n;
    return buf;
}
#else
#include <dirent.h>

char* ely_file_list_dir(const char* path, int* count) {
    DIR* d = opendir(path);
    if (!d) {
        if (count) *count = 0;
        return NULL;
    }
    int n = 0;
    int cap = 32;
    char** names = (char**)malloc(cap * sizeof(char*));
    struct dirent* de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        if (n >= cap) {
            cap *= 2;
            names = (char**)realloc(names, cap * sizeof(char*));
        }
        names[n++] = strdup(de->d_name);
    }
    closedir(d);

    size_t total = sizeof(int) + n * sizeof(char*);
    for (int i = 0; i < n; i++)
        total += strlen(names[i]) + 1;

    char* buf = (char*)malloc(total);
    *(int*)buf = n;
    char** ptrs = (char**)(buf + sizeof(int));
    char* strs = buf + sizeof(int) + n * sizeof(char*);
    for (int i = 0; i < n; i++) {
        strcpy(strs, names[i]);
        ptrs[i] = strs;
        strs += strlen(names[i]) + 1;
        free(names[i]);
    }
    free(names);
    if (count) *count = n;
    return buf;
}
#endif

/* ---------------- Temp files ---------------- */

char* ely_file_tmpnam(void) {
#ifdef _WIN32
    char* buf = (char*)malloc(L_tmpnam);
    tmpnam_s(buf, L_tmpnam);
    return buf;
#else
    char buf[L_tmpnam];
    tmpnam(buf);
    return strdup(buf);
#endif
}

/* ---------------- File time ---------------- */

long long ely_file_mtime(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (long long)st.st_mtime;
}

long long ely_file_ctime(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
#ifdef _WIN32
    return (long long)st.st_ctime;
#else
    return (long long)st.st_ctim.tv_sec;
#endif
}