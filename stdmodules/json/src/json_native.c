/**
 * json_native.c — Native C implementation for the JSON module.
 *
 * Provides JSON parsing (string→ely_value*) and serialization (ely_value*→string).
 * Uses the Ely runtime type system (ely_value, ely_dict, ely_arr, etc.).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* Ely runtime headers */
extern char* ely_str_dup(const char* s);
extern size_t ely_str_len(const char* s);

/* ely_value is opaque — forward declare and use via runtime functions */
typedef struct ely_value ely_value;

/* Declare runtime functions we need */
extern ely_value* ely_dictify(const char* json);
extern char* ely_dict_to_json(ely_value* v);

extern ely_value* ely_value_new_null(void);
extern ely_value* ely_value_new_bool(int b);
extern ely_value* ely_value_new_int(long long i);
extern ely_value* ely_value_new_double(double d);
extern ely_value* ely_value_new_string(const char* s);
extern ely_value* ely_value_new_dict(void);
extern ely_value* ely_value_new_arr(void);

extern void ely_dict_set_strkey(ely_value* dict, const char* key, ely_value* val);
extern void ely_arr_push(ely_value* arr, ely_value* val);

/* ---------------- JSON Parser State ---------------- */

typedef struct {
    const char* s;
    int pos;
    int len;
} JsonParser;

static void skip_ws(JsonParser* p) {
    while (p->pos < p->len && isspace((unsigned char)p->s[p->pos]))
        p->pos++;
}

static char peek(JsonParser* p) {
    if (p->pos >= p->len) return '\0';
    return p->s[p->pos];
}

static char advance(JsonParser* p) {
    if (p->pos >= p->len) return '\0';
    return p->s[p->pos++];
}

/* Forward */
static ely_value* parse_value(JsonParser* p);

/* Parse JSON string — returns ely_value* AND fills raw_buf with the C string */
static ely_value* parse_string_val(JsonParser* p, char* raw_out, int raw_buf_size) {
    advance(p); /* skip opening '"' */
    int bi = 0;

    while (p->pos < p->len) {
        char c = advance(p);
        if (c == '"') {
            if (raw_out && bi < raw_buf_size) raw_out[bi] = '\0';
            if (raw_out && bi < raw_buf_size) {
                raw_out[bi] = '\0';
                return ely_value_new_string(raw_out);
            }
            /* if no raw_out buffer, use a static buf (not thread-safe but ok for now) */
            static char static_buf[4096];
            memcpy(static_buf, raw_out ? raw_out : "", bi);
            static_buf[bi] = '\0';
            return ely_value_new_string(static_buf);
        }
        if (c == '\\') {
            char esc = advance(p);
            switch (esc) {
                case '"':  c = '"';  break;
                case '\\': c = '\\'; break;
                case '/':  c = '/';  break;
                case 'b':  c = '\b'; break;
                case 'f':  c = '\f'; break;
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                case 't':  c = '\t'; break;
                case 'u':
                    p->pos += 4;
                    c = '?';
                    break;
                default:
                    c = esc;
                    break;
            }
        }
        if (raw_out) {
            if (bi < raw_buf_size - 1) raw_out[bi] = c;
        }
        bi++;
    }
    if (raw_out && bi < raw_buf_size) raw_out[bi] = '\0';
    if (raw_out && bi < raw_buf_size) {
        raw_out[bi] = '\0';
        return ely_value_new_string(raw_out);
    }
    static char static_buf2[4096];
    if (raw_out) memcpy(static_buf2, raw_out, bi < 4096 ? bi : 4095);
    static_buf2[bi < 4096 ? bi : 4095] = '\0';
    return ely_value_new_string(static_buf2);
}

/* Parse JSON number */
static ely_value* parse_number(JsonParser* p) {
    int start = p->pos;
    int is_float = 0;

    if (peek(p) == '-') advance(p);

    while (p->pos < p->len && isdigit((unsigned char)peek(p)))
        advance(p);

    if (p->pos < p->len && peek(p) == '.') {
        is_float = 1;
        advance(p);
        while (p->pos < p->len && isdigit((unsigned char)peek(p)))
            advance(p);
    }

    if (p->pos < p->len && (peek(p) == 'e' || peek(p) == 'E')) {
        is_float = 1;
        advance(p);
        if (peek(p) == '+' || peek(p) == '-') advance(p);
        while (p->pos < p->len && isdigit((unsigned char)peek(p)))
            advance(p);
    }

    int len = p->pos - start;
    char buf[256];
    if (len > 255) len = 255;
    memcpy(buf, p->s + start, len);
    buf[len] = '\0';

    if (is_float) {
        return ely_value_new_double(atof(buf));
    } else {
        return ely_value_new_int(atoll(buf));
    }
}

/* Parse JSON array */
static ely_value* parse_array(JsonParser* p) {
    advance(p); /* skip '[' */
    ely_value* arr = ely_value_new_arr();

    skip_ws(p);
    if (peek(p) == ']') {
        advance(p);
        return arr;
    }

    while (1) {
        skip_ws(p);
        ely_value* val = parse_value(p);
        if (val) ely_arr_push(arr, val);
        skip_ws(p);
        if (peek(p) == ']') {
            advance(p);
            return arr;
        }
        if (peek(p) == ',') {
            advance(p);
        }
    }
}

/* Parse JSON object */
static ely_value* parse_object(JsonParser* p) {
    advance(p); /* skip '{' */
    ely_value* dict = ely_value_new_dict();

    skip_ws(p);
    if (peek(p) == '}') {
        advance(p);
        return dict;
    }

    while (1) {
        skip_ws(p);
        if (peek(p) != '"') break;

        /* Parse key as raw C string */
        char key_buf[4096];
        parse_string_val(p, key_buf, sizeof(key_buf));

        skip_ws(p);
        if (peek(p) != ':') break;
        advance(p); /* skip ':' */

        skip_ws(p);
        ely_value* val = parse_value(p);
        if (val) ely_dict_set_strkey(dict, key_buf, val);

        skip_ws(p);
        if (peek(p) == '}') {
            advance(p);
            return dict;
        }
        if (peek(p) == ',') {
            advance(p);
        } else {
            break;
        }
    }

    return dict;
}

/* Parse a string value (for top-level string parsing) */
static ely_value* parse_string_top(JsonParser* p) {
    char buf[4096];
    return parse_string_val(p, buf, sizeof(buf));
}

static ely_value* parse_value(JsonParser* p) {
    skip_ws(p);
    char c = peek(p);

    if (c == '{') return parse_object(p);
    if (c == '[') return parse_array(p);
    if (c == '"') return parse_string_top(p);

    if (c == 't' || c == 'f') {
        if (strncmp(p->s + p->pos, "true", 4) == 0) {
            p->pos += 4;
            return ely_value_new_bool(1);
        }
        if (strncmp(p->s + p->pos, "false", 5) == 0) {
            p->pos += 5;
            return ely_value_new_bool(0);
        }
        return ely_value_new_null();
    }

    if (c == 'n') {
        if (strncmp(p->s + p->pos, "null", 4) == 0) {
            p->pos += 4;
            return ely_value_new_null();
        }
        return ely_value_new_null();
    }

    if (c == '-' || isdigit((unsigned char)c)) {
        return parse_number(p);
    }

    return ely_value_new_null();
}

/* ---------------- Public API ---------------- */

/**
 * json_parse_str — Parse a JSON string and return an ely_value* tree.
 */
ely_value* json_parse_str(const char* s) {
    if (!s) return ely_value_new_null();

    JsonParser p;
    p.s = s;
    p.pos = 0;
    p.len = (int)strlen(s);

    skip_ws(&p);
    if (p.pos >= p.len) return ely_value_new_null();

    ely_value* result = parse_value(&p);
    return result ? result : ely_value_new_null();
}

/**
 * json_value_to_str — Serialize an ely_value* to a JSON string.
 * Uses the runtime's ely_dict_to_json.
 */
char* json_value_to_str(void* value) {
    if (!value) return ely_str_dup("null");
    return ely_dict_to_json((ely_value*)value);
}

/**
 * json_is_valid — Check if a string is valid JSON.
 * Returns 1 if valid, 0 otherwise.
 */
int json_is_valid(const char* s) {
    if (!s) return 0;

    JsonParser p;
    p.s = s;
    p.pos = 0;
    p.len = (int)strlen(s);

    skip_ws(&p);
    if (p.pos >= p.len) return 0;

    ely_value* result = parse_value(&p);
    skip_ws(&p);

    if (result && p.pos >= p.len) {
        return 1;
    }
    return 0;
}