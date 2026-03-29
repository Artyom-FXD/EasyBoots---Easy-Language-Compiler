#include "ely_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif

// -------------------------------------------------------------------
// Собственные strtoll/strtoull для Windows
// -------------------------------------------------------------------
#ifndef _WIN32
#define my_strtoll strtoll
#define my_strtoull strtoull
#else
static long long my_strtoll(char *nptr, char **endptr, int base) {
    long long val = 0;
    int sign = 1;
    if (base == 0) {
        if (*nptr == '0') {
            if (nptr[1] == 'x' || nptr[1] == 'X') base = 16;
            else base = 8;
        } else base = 10;
    }
    while (*nptr == ' ' || *nptr == '\t') nptr++;
    if (*nptr == '-') { sign = -1; nptr++; }
    else if (*nptr == '+') nptr++;
    if (base == 16 && *nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) nptr += 2;
    while (*nptr) {
        int digit;
        if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
        else if (base == 16 && *nptr >= 'a' && *nptr <= 'f') digit = *nptr - 'a' + 10;
        else if (base == 16 && *nptr >= 'A' && *nptr <= 'F') digit = *nptr - 'A' + 10;
        else break;
        if (digit >= base) break;
        val = val * base + digit;
        nptr++;
    }
    if (endptr) *endptr = (char*)nptr;
    return sign * val;
}

static unsigned long long my_strtoull(char *nptr, char **endptr, int base) {
    unsigned long long val = 0;
    if (base == 0) {
        if (*nptr == '0') {
            if (nptr[1] == 'x' || nptr[1] == 'X') base = 16;
            else base = 8;
        } else base = 10;
    }
    while (*nptr == ' ' || *nptr == '\t') nptr++;
    if (*nptr == '-') nptr++;
    else if (*nptr == '+') nptr++;
    if (base == 16 && *nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) nptr += 2;
    while (*nptr) {
        int digit;
        if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
        else if (base == 16 && *nptr >= 'a' && *nptr <= 'f') digit = *nptr - 'a' + 10;
        else if (base == 16 && *nptr >= 'A' && *nptr <= 'F') digit = *nptr - 'A' + 10;
        else break;
        if (digit >= base) break;
        val = val * base + digit;
        nptr++;
    }
    if (endptr) *endptr = (char*)nptr;
    return val;
}
#endif

// ------------------------ Консоль ------------------------
void ely_print(ely_str str) { if (str) fputs(str, stdout); }
void ely_print_int(ely_int n) { printf("%d", n); }
void ely_print_uint(ely_uint n) { printf("%u", n); }
void ely_print_more(ely_more n) { printf("%lld", n); }
void ely_print_umore(ely_umore n) { printf("%llu", n); }
void ely_print_flt(ely_flt f) { printf("%f", f); }
void ely_print_double(ely_double d) { printf("%lf", d); }
void ely_print_bool(ely_bool b) { fputs(b ? "true" : "false", stdout); }
void ely_print_char(ely_char c) { putchar(c); }
void ely_print_byte(ely_byte b) { printf("%d", (int)b); }
void ely_print_ubyte(ely_ubyte b) { printf("%u", (unsigned int)b); }
void ely_println(ely_str str) {
    if (str) fputs(str, stdout);
    putchar('\n');
    fflush(stdout);
}

ely_str ely_input(void) {
    static char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len && buffer[len-1] == '\n') buffer[len-1] = '\0';
        char* res = ely_alloc(len + 1);
        if (res) strcpy(res, buffer);
        return res;
    }
    return NULL;
}

ely_str ely_input_prompt(ely_str prompt) {
    if (prompt) ely_print(prompt);
    return ely_input();
}

// ------------------------ Преобразования строк в числа ------------------------
ely_int ely_str_to_int(ely_str str) {
    if (!str) return 0;
    long long v = my_strtoll(str, NULL, 10);
    return (ely_int)v;
}
ely_uint ely_str_to_uint(ely_str str) {
    if (!str) return 0;
    unsigned long long v = my_strtoull(str, NULL, 10);
    return (ely_uint)v;
}
ely_more ely_str_to_more(ely_str str) {
    if (!str) return 0;
    return my_strtoll(str, NULL, 10);
}
ely_umore ely_str_to_umore(ely_str str) {
    if (!str) return 0;
    return my_strtoull(str, NULL, 10);
}
ely_flt ely_str_to_flt(ely_str str) {
    if (!str) return 0.0f;
    return (ely_flt)strtod(str, NULL);
}
ely_double ely_str_to_double(ely_str str) {
    if (!str) return 0.0;
    return strtod(str, NULL);
}

// ------------------------ Преобразования чисел в строки ------------------------
static ely_str _int_to_str(long long n) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lld", n);
    if (len < 0) return NULL;
    char* res = ely_alloc(len + 1);
    if (res) memcpy(res, buf, len + 1);
    return res;
}
static ely_str _uint_to_str(unsigned long long n) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%llu", n);
    if (len < 0) return NULL;
    char* res = ely_alloc(len + 1);
    if (res) memcpy(res, buf, len + 1);
    return res;
}
ely_str ely_int_to_str(ely_int n) { return _int_to_str(n); }
ely_str ely_uint_to_str(ely_uint n) { return _uint_to_str(n); }
ely_str ely_more_to_str(ely_more n) { return _int_to_str(n); }
ely_str ely_umore_to_str(ely_umore n) { return _uint_to_str(n); }
ely_str ely_flt_to_str(ely_flt f) {
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%g", (double)f);
    if (len < 0) return NULL;
    char* res = ely_alloc(len + 1);
    if (res) memcpy(res, buf, len + 1);
    return res;
}
ely_str ely_double_to_str(ely_double d) {
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%g", d);
    if (len < 0) return NULL;
    char* res = ely_alloc(len + 1);
    if (res) memcpy(res, buf, len + 1);
    return res;
}
ely_str ely_bool_to_str(ely_bool b) {
    char* s = b ? "true" : "false";
    char* res = ely_alloc(strlen(s) + 1);
    if (res) strcpy(res, s);
    return res;
}

// ------------------------ Строки ------------------------
size_t ely_str_len(ely_str str) { return str ? strlen(str) : 0; }
ely_str ely_str_dup(ely_str str) {
    if (!str) return NULL;
    char* dup = ely_alloc(strlen(str) + 1);
    if (dup) strcpy(dup, str);
    return dup;
}
ely_str ely_str_concat(ely_str a, ely_str b) {
    if (!a && !b) return NULL;
    size_t la = a ? strlen(a) : 0;
    size_t lb = b ? strlen(b) : 0;
    char* res = ely_alloc(la + lb + 1);
    if (!res) return NULL;
    if (la) memcpy(res, a, la);
    if (lb) memcpy(res + la, b, lb);
    res[la+lb] = '\0';
    return res;
}
int ely_str_cmp(ely_str a, ely_str b) {
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp(a, b);
}
ely_str ely_str_substr(ely_str str, size_t start, size_t len) {
    if (!str) return NULL;
    size_t slen = strlen(str);
    if (start >= slen) return ely_str_dup("");
    if (start + len > slen) len = slen - start;
    char* res = ely_alloc(len + 1);
    if (!res) return NULL;
    memcpy(res, str + start, len);
    res[len] = '\0';
    return res;
}
ely_str ely_str_trim(ely_str str) {
    if (!str) return NULL;
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n')) str++;
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' || str[len-1] == '\n')) len--;
    char* res = ely_alloc(len + 1);
    if (!res) return NULL;
    memcpy(res, str, len);
    res[len] = '\0';
    return res;
}
ely_str ely_str_replace(ely_str str, ely_str old, ely_str new) {
    if (!str || !old) return ely_str_dup(str);
    size_t old_len = strlen(old);
    if (old_len == 0) return ely_str_dup(str);
    size_t new_len = new ? strlen(new) : 0;
    size_t count = 0;
    char* pos = str;
    while ((pos = strstr(pos, old))) { count++; pos += old_len; }
    if (count == 0) return ely_str_dup(str);
    size_t result_len = strlen(str) + count * (new_len - old_len);
    char* res = ely_alloc(result_len + 1);
    if (!res) return NULL;
    char* out = res;
    pos = str;
    while (*pos) {
        char* found = strstr(pos, old);
        if (found) {
            size_t before = found - pos;
            memcpy(out, pos, before);
            out += before;
            if (new) { memcpy(out, new, new_len); out += new_len; }
            pos = found + old_len;
        } else {
            strcpy(out, pos);
            break;
        }
    }
    return res;
}

// ------------------------ Математика ------------------------
ely_int ely_abs_int(ely_int n) { return n < 0 ? -n : n; }
ely_more ely_abs_more(ely_more n) { return n < 0 ? -n : n; }
ely_double ely_fabs(ely_double x) { return fabs(x); }
ely_int ely_min_int(ely_int a, ely_int b) { return a < b ? a : b; }
ely_more ely_min_more(ely_more a, ely_more b) { return a < b ? a : b; }
ely_double ely_min_double(ely_double a, ely_double b) { return a < b ? a : b; }
ely_int ely_max_int(ely_int a, ely_int b) { return a > b ? a : b; }
ely_more ely_max_more(ely_more a, ely_more b) { return a > b ? a : b; }
ely_double ely_max_double(ely_double a, ely_double b) { return a > b ? a : b; }
ely_double ely_pow(ely_double base, ely_double exp) { return pow(base, exp); }
ely_double ely_sqrt(ely_double x) { return sqrt(x); }
ely_double ely_sin(ely_double x) { return sin(x); }
ely_double ely_cos(ely_double x) { return cos(x); }
ely_double ely_tan(ely_double x) { return tan(x); }

// ------------------------ Случайные числа ------------------------
static unsigned int rand_seed = 1;
void ely_srand(ely_uint seed) { rand_seed = seed; }
ely_int ely_rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (ely_int)((rand_seed >> 16) & 0x7FFF);
}
ely_double ely_rand_double(void) {
    return (ely_double)ely_rand() / 32767.0;
}

// ------------------------ Время ------------------------
void ely_sleep(ely_uint milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}
ely_more ely_time_now(void) {
    return (ely_more)time(NULL);
}
double ely_time_diff(ely_more start, ely_more end) {
    return (double)(end - start);
}

// ------------------------ Файлы ------------------------
typedef struct ely_file {
    FILE* fp;
} ely_file;

ely_file* ely_file_open(char* path, char* mode) {
    FILE* fp = fopen(path, mode);
    if (!fp) return NULL;
    ely_file* f = ely_alloc(sizeof(ely_file));
    if (!f) { fclose(fp); return NULL; }
    f->fp = fp;
    return f;
}
void ely_file_close(ely_file* f) {
    if (f) {
        if (f->fp) fclose(f->fp);
        ely_free(f);
    }
}
int ely_file_write(ely_file* f, char* data, size_t len) {
    if (!f || !f->fp) return -1;
    return (fwrite(data, 1, len, f->fp) == len) ? 0 : -1;
}
char* ely_file_read(ely_file* f, size_t* out_len) {
    if (!f || !f->fp) return NULL;
    char* result = NULL;
    size_t total = 0, cap = 0;
    char buf[4096];
    while (1) {
        size_t n = fread(buf, 1, sizeof(buf), f->fp);
        if (n == 0) break;
        if (total + n > cap) {
            cap = (total + n) * 2 + 1024;
            char* new_res = realloc(result, cap);
            if (!new_res) { free(result); return NULL; }
            result = new_res;
        }
        memcpy(result + total, buf, n);
        total += n;
    }
    if (out_len) *out_len = total;
    if (total == 0) {
        free(result);
        return NULL;
    }
    char* final = realloc(result, total + 1);
    if (final) result = final;
    result[total] = '\0';
    return result;
}
int ely_file_exists(char* path) {
    FILE* f = fopen(path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}
char* ely_file_read_all(char* path, size_t* out_len) {
    ely_file* f = ely_file_open(path, "rb");
    if (!f) return NULL;
    char* data = ely_file_read(f, out_len);
    ely_file_close(f);
    return data;
}
int ely_file_remove(char* path) { return remove(path); }
int ely_file_rename(char* old, char* new) { return rename(old, new); }

// ------------------------ Пути ------------------------
ely_str ely_path_join(ely_str a, ely_str b) {
    if (!a && !b) return NULL;
    if (!a) return ely_str_dup(b);
    if (!b) return ely_str_dup(a);
    size_t la = strlen(a);
    size_t lb = strlen(b);
    char* res = ely_alloc(la + lb + 2);
    if (!res) return NULL;
    strcpy(res, a);
    if (la > 0 && res[la-1] != '/' && res[la-1] != '\\')
        strcat(res, "/");
    strcat(res, b);
    return res;
}
ely_str ely_path_basename(ely_str path) {
    if (!path) return NULL;
    char* sep = strrchr(path, '/');
    if (!sep) sep = strrchr(path, '\\');
    if (!sep) return ely_str_dup(path);
    return ely_str_dup(sep + 1);
}
ely_str ely_path_dirname(ely_str path) {
    if (!path) return NULL;
    char* sep = strrchr(path, '/');
    if (!sep) sep = strrchr(path, '\\');
    if (!sep) return ely_str_dup(".");
    size_t len = sep - path;
    if (len == 0) return ely_str_dup(".");
    char* res = ely_alloc(len + 1);
    if (!res) return NULL;
    memcpy(res, path, len);
    res[len] = '\0';
    return res;
}
int ely_path_is_absolute(ely_str path) {
    if (!path) return 0;
    if (path[0] == '/' || path[0] == '\\') return 1;
#ifdef _WIN32
    if (path[0] && path[1] == ':') return 1;
#endif
    return 0;
}

// ------------------------ Динамические библиотеки ------------------------
#ifdef _WIN32
#define LIB_HANDLE HMODULE
#define LIB_LOAD(path) LoadLibraryA(path)
#define LIB_GET(lib, name) GetProcAddress((HMODULE)lib, name)
#define LIB_CLOSE(lib) FreeLibrary((HMODULE)lib)
#else
#define LIB_HANDLE void*
#define LIB_LOAD(path) dlopen(path, RTLD_LAZY)
#define LIB_GET(lib, name) dlsym(lib, name)
#define LIB_CLOSE(lib) dlclose(lib)
#endif

void* ely_load_library(char* path) {
    if (!path) return NULL;
    return (void*)LIB_LOAD(path);
}
void* ely_get_function(void* lib, char* name) {
    if (!lib || !name) return NULL;
    return LIB_GET(lib, name);
}
void ely_close_library(void* lib) {
    if (lib) LIB_CLOSE(lib);
}
int ely_call_int_int(void* func, int a, int b) {
    if (!func) return 0;
    int (*f)(int, int) = (int (*)(int, int))func;
    return f(a, b);
}
double ely_call_double_double(void* func, double a) {
    if (!func) return 0.0;
    double (*f)(double) = (double (*)(double))func;
    return f(a);
}
double ely_call_double_double_double(void* func, double a, double b) {
    if (!func) return 0.0;
    double (*f)(double, double) = (double (*)(double, double))func;
    return f(a, b);
}
char* ely_call_str_void(void* func) {
    if (!func) return NULL;
    char* (*f)(void) = (char* (*)(void))func;
    return f();
}

// ------------------------ Память ------------------------
void* ely_alloc(size_t size) { return malloc(size); }
void ely_free(void* ptr) { free(ptr); }

// ------------------------ JSON сериализация (адаптирована под arr/dict) ------------------------
static char* _jsonify_string(char* s) {
    if (!s) return ely_str_dup("null");
    size_t len = strlen(s);
    char* out = ely_alloc(len * 2 + 3);
    char* p = out;
    *p++ = '"';
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '"' || c == '\\') {
            *p++ = '\\';
            *p++ = c;
        } else if (c == '\n') {
            *p++ = '\\';
            *p++ = 'n';
        } else if (c == '\r') {
            *p++ = '\\';
            *p++ = 'r';
        } else if (c == '\t') {
            *p++ = '\\';
            *p++ = 't';
        } else {
            *p++ = c;
        }
    }
    *p++ = '"';
    *p = '\0';
    char* result = ely_str_dup(out);
    ely_free(out);
    return result;
}

char* ely_dict_to_json(dict* d) {
    if (!d) return ely_str_dup("null");
    char* result = ely_str_dup("{");
    int first = 1;
    arr* keys = dict_keys(d);
    if (keys) {
        for (size_t i = 0; i < arr_len(keys); i++) {
            char** key_ptr = (char**)arr_get(keys, i);
            if (!key_ptr) continue;
            char* key = *key_ptr;
            void* value_ptr = dict_get(d, &key);
            if (!value_ptr) continue;
            char** val_str = (char**)value_ptr;
            if (!first) result = ely_str_concat(result, ",");
            first = 0;
            char* key_json = _jsonify_string(key);
            result = ely_str_concat(result, key_json);
            ely_free(key_json);
            result = ely_str_concat(result, ":");
            char* val_json = _jsonify_string(val_str ? *val_str : "null");
            result = ely_str_concat(result, val_json);
            ely_free(val_json);
        }
        arr_free(keys);
    }
    result = ely_str_concat(result, "}");
    return result;
}

char* ely_array_to_json(arr* a) {
    if (!a) return ely_str_dup("null");
    char* result = ely_str_dup("[");
    for (size_t i = 0; i < arr_len(a); i++) {
        if (i > 0) result = ely_str_concat(result, ",");
        if (a->elem_size == sizeof(int)) {
            int val = *(int*)arr_get(a, i);
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", val);
            char* val_json = ely_str_dup(buf);
            result = ely_str_concat(result, val_json);
            ely_free(val_json);
        } else if (a->elem_size == sizeof(long long)) {
            long long val = *(long long*)arr_get(a, i);
            char buf[32];
            snprintf(buf, sizeof(buf), "%lld", val);
            char* val_json = ely_str_dup(buf);
            result = ely_str_concat(result, val_json);
            ely_free(val_json);
        } else if (a->elem_size == sizeof(double)) {
            double val = *(double*)arr_get(a, i);
            char buf[64];
            snprintf(buf, sizeof(buf), "%g", val);
            char* val_json = ely_str_dup(buf);
            result = ely_str_concat(result, val_json);
            ely_free(val_json);
        } else if (a->elem_size == sizeof(char*)) {
            char** valp = (char**)arr_get(a, i);
            char* val = valp ? *valp : NULL;
            char* val_json = _jsonify_string(val ? val : "null");
            result = ely_str_concat(result, val_json);
            ely_free(val_json);
        } else {
            result = ely_str_concat(result, "null");
        }
    }
    result = ely_str_concat(result, "]");
    return result;
}

char* ely_jsonify(dict* d) {
    return ely_dict_to_json(d);
}

// ------------------------ Парсинг JSON (ely_dictify) ------------------------
typedef struct json_parser {
    char* str;
    size_t pos;
    size_t len;
} json_parser;

static void skip_whitespace(json_parser* p) {
    while (p->pos < p->len && isspace(p->str[p->pos])) p->pos++;
}
static int peek(json_parser* p) {
    if (p->pos >= p->len) return 0;
    return p->str[p->pos];
}
static int consume(json_parser* p, char expected) {
    skip_whitespace(p);
    if (p->pos < p->len && p->str[p->pos] == expected) {
        p->pos++;
        return 1;
    }
    return 0;
}
static char* parse_string(json_parser* p) {
    if (!consume(p, '"')) return NULL;
    size_t start = p->pos;
    while (p->pos < p->len && p->str[p->pos] != '"') {
        if (p->str[p->pos] == '\\') p->pos++;
        p->pos++;
    }
    if (p->pos >= p->len) return NULL;
    size_t end = p->pos;
    consume(p, '"');
    size_t len = end - start;
    char* buf = ely_alloc(len + 1);
    if (!buf) return NULL;
    memcpy(buf, p->str + start, len);
    buf[len] = '\0';
    return buf;
}
static char* parse_number(json_parser* p) {
    char* start = p->str + p->pos;
    while (p->pos < p->len && (isdigit(p->str[p->pos]) || p->str[p->pos] == '.' || p->str[p->pos] == '-' || p->str[p->pos] == 'e' || p->str[p->pos] == 'E')) p->pos++;
    size_t len = p->pos - (start - p->str);
    char* buf = ely_alloc(len + 1);
    if (!buf) return NULL;
    memcpy(buf, start, len);
    buf[len] = '\0';
    return buf;
}
static char* parse_bool(json_parser* p) {
    if (strncmp(p->str + p->pos, "true", 4) == 0) {
        p->pos += 4;
        return ely_str_dup("true");
    } else if (strncmp(p->str + p->pos, "false", 5) == 0) {
        p->pos += 5;
        return ely_str_dup("false");
    }
    return NULL;
}
static char* parse_null(json_parser* p) {
    if (strncmp(p->str + p->pos, "null", 4) == 0) {
        p->pos += 4;
        return ely_str_dup("null");
    }
    return NULL;
}
static dict* parse_object(json_parser* p);
static arr* parse_array(json_parser* p);
static char* parse_value(json_parser* p);

dict* ely_dictify(ely_str json_str) {
    if (!json_str) return NULL;
    json_parser parser = { json_str, 0, strlen(json_str) };
    skip_whitespace(&parser);
    if (!consume(&parser, '{')) return NULL;
    dict* d = dict_new_str(sizeof(char*));
    while (1) {
        skip_whitespace(&parser);
        if (peek(&parser) == '}') {
            consume(&parser, '}');
            break;
        }
        char* key = parse_string(&parser);
        if (!key) { dict_free(d); return NULL; }
        skip_whitespace(&parser);
        if (!consume(&parser, ':')) { ely_free(key); dict_free(d); return NULL; }
        char* value = parse_value(&parser);
        if (!value) { ely_free(key); dict_free(d); return NULL; }
        dict_set_str(d, key, &value);
        ely_free(key);
        skip_whitespace(&parser);
        if (peek(&parser) == ',') consume(&parser, ',');
        else if (peek(&parser) == '}') continue;
        else { dict_free(d); return NULL; }
    }
    return d;
}

static char* parse_value(json_parser* p) {
    skip_whitespace(p);
    char c = peek(p);
    if (c == '"') {
        return parse_string(p);
    } else if (c == '-' || isdigit(c)) {
        return parse_number(p);
    } else if (c == 't' || c == 'f') {
        return parse_bool(p);
    } else if (c == 'n') {
        return parse_null(p);
    } else if (c == '{') {
        dict* obj = parse_object(p);
        if (!obj) return NULL;
        char* json = ely_dict_to_json(obj);
        dict_free(obj);
        return json;
    } else if (c == '[') {
        arr* a = parse_array(p);
        if (!a) return NULL;
        char* json = ely_array_to_json(a);
        arr_free(a);
        return json;
    }
    return NULL;
}

static dict* parse_object(json_parser* p) {
    if (!consume(p, '{')) return NULL;
    dict* d = dict_new_str(sizeof(char*));
    while (1) {
        skip_whitespace(p);
        if (peek(p) == '}') {
            consume(p, '}');
            break;
        }
        char* key = parse_string(p);
        if (!key) { dict_free(d); return NULL; }
        skip_whitespace(p);
        if (!consume(p, ':')) { ely_free(key); dict_free(d); return NULL; }
        char* value = parse_value(p);
        if (!value) { ely_free(key); dict_free(d); return NULL; }
        dict_set_str(d, key, &value);
        ely_free(key);
        skip_whitespace(p);
        if (peek(p) == ',') consume(p, ',');
        else if (peek(p) == '}') continue;
        else { dict_free(d); return NULL; }
    }
    return d;
}

static arr* parse_array(json_parser* p) {
    if (!consume(p, '[')) return NULL;
    arr* a = arr_new(sizeof(char*));
    while (1) {
        skip_whitespace(p);
        if (peek(p) == ']') {
            consume(p, ']');
            break;
        }
        char* value = parse_value(p);
        if (!value) { arr_free(a); return NULL; }
        arr_push(a, &value);
        skip_whitespace(p);
        if (peek(p) == ',') consume(p, ',');
        else if (peek(p) == ']') continue;
        else { arr_free(a); return NULL; }
    }
    return a;
}

int ely_file_write_all(char* path, char* data, size_t len) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return (written == len) ? 0 : -1;
}

// ------------------------ ely_value implementation (остаётся без изменений, но ссылается на arr/dict) ------------------------
typedef enum {
    ely_VALUE_NULL,
    ely_VALUE_BOOL,
    ely_VALUE_INT,
    ely_VALUE_DOUBLE,
    ely_VALUE_STRING,
    ely_VALUE_ARRAY,
    ely_VALUE_OBJECT
} ely_value_type;

typedef struct ely_value {
    ely_value_type type;
    union {
        int bool_val;
        long long int_val;
        double double_val;
        char* string_val;
        arr* array_val;
        dict* object_val;
    } u;
} ely_value;

ely_value* ely_value_new_null(void) {
    ely_value* v = (ely_value*)ely_alloc(sizeof(ely_value));
    if (!v) return NULL;
    v->type = ely_VALUE_NULL;
    return v;
}
ely_value* ely_value_new_bool(int b) {
    ely_value* v = (ely_value*)ely_alloc(sizeof(ely_value));
    if (!v) return NULL;
    v->type = ely_VALUE_BOOL;
    v->u.bool_val = b;
    return v;
}
ely_value* ely_value_new_int(long long i) {
    ely_value* v = (ely_value*)ely_alloc(sizeof(ely_value));
    if (!v) return NULL;
    v->type = ely_VALUE_INT;
    v->u.int_val = i;
    return v;
}
ely_value* ely_value_new_double(double d) {
    ely_value* v = (ely_value*)ely_alloc(sizeof(ely_value));
    if (!v) return NULL;
    v->type = ely_VALUE_DOUBLE;
    v->u.double_val = d;
    return v;
}
ely_value* ely_value_new_string(char* s) {
    ely_value* v = (ely_value*)ely_alloc(sizeof(ely_value));
    if (!v) return NULL;
    v->type = ely_VALUE_STRING;
    v->u.string_val = s ? ely_str_dup(s) : NULL;
    return v;
}
ely_value* ely_value_new_array(arr* a) {
    ely_value* v = (ely_value*)ely_alloc(sizeof(ely_value));
    if (!v) return NULL;
    v->type = ely_VALUE_ARRAY;
    v->u.array_val = a;
    return v;
}
ely_value* ely_value_new_object(dict* d) {
    ely_value* v = (ely_value*)ely_alloc(sizeof(ely_value));
    if (!v) return NULL;
    v->type = ely_VALUE_OBJECT;
    v->u.object_val = d;
    return v;
}
void ely_value_free(ely_value* v) {
    if (!v) return;
    switch (v->type) {
        case ely_VALUE_STRING: if (v->u.string_val) ely_free(v->u.string_val); break;
        case ely_VALUE_ARRAY: if (v->u.array_val) arr_free(v->u.array_val); break;
        case ely_VALUE_OBJECT: if (v->u.object_val) dict_free(v->u.object_val); break;
        default: break;
    }
    ely_free(v);
}

static char* _value_to_json(const ely_value* v) {
    if (!v) return ely_str_dup("null");
    switch (v->type) {
        case ely_VALUE_NULL: return ely_str_dup("null");
        case ely_VALUE_BOOL: return ely_str_dup(v->u.bool_val ? "true" : "false");
        case ely_VALUE_INT: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%lld", v->u.int_val);
            return ely_str_dup(buf);
        }
        case ely_VALUE_DOUBLE: {
            char buf[64];
            snprintf(buf, sizeof(buf), "%g", v->u.double_val);
            return ely_str_dup(buf);
        }
        case ely_VALUE_STRING:
            return _jsonify_string(v->u.string_val ? v->u.string_val : "");
        case ely_VALUE_ARRAY:
            return ely_array_to_json(v->u.array_val);
        case ely_VALUE_OBJECT:
            return ely_dict_to_json(v->u.object_val);
        default:
            return ely_str_dup("null");
    }
}
char* ely_value_to_json(ely_value* v) { return _value_to_json(v); }

ely_value* ely_value_from_json(char* json, size_t* pos) {
    (void)pos;
    // упрощённая реализация
    dict* d = ely_dictify(json);
    if (d) return ely_value_new_object(d);
    return NULL;
}