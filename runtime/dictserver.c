#include "dictserver.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct DictServer {
    char* path;           // путь к файлу (для сохранения)
    dict* root;      // корневой объект
};

static char* next_segment(char* path, char** next) {
    char* dot = strchr(path, '.');
    if (dot) {
        size_t len = dot - path;
        char* seg = ely_alloc(len + 1);
        memcpy(seg, path, len);
        seg[len] = '\0';
        *next = dot + 1;
        return seg;
    } else {
        char* seg = ely_str_dup(path);
        *next = NULL;
        return seg;
    }
}

// Получение узла по пути (создание, если create=1)
static void* get_node(DictServer* ds, char* path, int create, int* is_array) {
    if (!path || !*path) return ds->root;
    char* cur = path;
    void* current = ds->root;
    void* parent = NULL;
    char* last_seg = NULL;
    int last_is_array = 0;

    while (cur) {
        char* seg = next_segment(cur, &cur);
        // Если сегмент – число, то работаем с массивом
        int index = -1;
        char* endptr;
        long idx = strtol(seg, &endptr, 10);
        if (*endptr == '\0') {
            index = (int)idx;
            // текущий узел должен быть массивом
            if (!current) {
                if (!create) { ely_free(seg); return NULL; }
                // создать массив
                current = arr_new(sizeof(void*));
                if (parent) {
                    if (last_is_array) {
                        // parent – массив, last_seg – индекс
                        arr_set(parent, (size_t)index, &current);
                    } else {
                        // parent – словарь, last_seg – ключ
                        dict_set_str(parent, last_seg, &current);
                    }
                } else {
                    ds->root = current;
                }
            }
            if (!current) { ely_free(seg); return NULL; }
            // Проверим, что current – массив
            // В нашей реализации мы не храним тип явно, поэтому будем предполагать, что если узел – arr*, то это массив.
            // Для простоты будем проверять, что это указатель на arr, но это ненадёжно. Лучше хранить тип.
            // Пока сделаем предположение.
            if (cur == NULL) {
                // последний сегмент – индекс, возвращаем элемент массива
                if (is_array) *is_array = 1;
                void* elem = arr_get(current, (size_t)index);
                if (elem) return *(void**)elem;
                if (create) {
                    void* new_elem = NULL;
                    arr_set(current, (size_t)index, &new_elem);
                    return &new_elem;
                }
                return NULL;
            } else {
                // переходим в элемент массива
                void* elem_ptr = arr_get(current, (size_t)index);
                if (!elem_ptr && create) {
                    void* new_elem = NULL;
                    arr_set(current, (size_t)index, &new_elem);
                    elem_ptr = &new_elem;
                }
                parent = current;
                last_seg = seg;
                last_is_array = 1;
                current = elem_ptr ? *(void**)elem_ptr : NULL;
                continue;
            }
        } else {
            // работаем со словарём
            if (!current) {
                if (!create) { ely_free(seg); return NULL; }
                // создать словарь
                current = dict_new_str(sizeof(void*));
                if (parent) {
                    if (last_is_array) {
                        arr_set(parent, (size_t)index, &current);
                    } else {
                        dict_set_str(parent, last_seg, &current);
                    }
                } else {
                    ds->root = current;
                }
            }
            if (!current) { ely_free(seg); return NULL; }
            if (cur == NULL) {
                // последний сегмент – ключ
                if (is_array) *is_array = 0;
                void* val = dict_get(current, seg);
                if (val) return val;
                if (create) {
                    void* new_val = NULL;
                    dict_set_str(current, seg, &new_val);
                    // возвращаем указатель на новое значение
                    return &new_val;
                }
                return NULL;
            } else {
                // переходим в словарь по ключу
                void* val = dict_get(current, seg);
                if (!val && create) {
                    void* new_val = NULL;
                    dict_set_str(current, seg, &new_val);
                    val = &new_val;
                }
                parent = current;
                last_seg = seg;
                last_is_array = 0;
                current = val ? *(void**)val : NULL;
                continue;
            }
        }
        ely_free(seg);
    }
    return NULL;
}

// Создание нового пустого сервера
static DictServer* new_dictserver(char* path) {
    DictServer* ds = ely_alloc(sizeof(DictServer));
    ds->path = path ? ely_str_dup(path) : NULL;
    ds->root = dict_new(sizeof(void*));
    return ds;
}

void DictServer_save(DictServer* ds) {
    if (!ds || !ds->path) return;
    ely_str json = dict_to_json(ds->root);
    FILE* f = fopen(ds->path, "w");
    if (f) {
        fputs(json, f);
        fclose(f);
    }
    ely_free(json);
}

// Реализация get-функций
ely_str DictServer_get_str(DictServer* ds, char* path) {
    void* node = get_node(ds, path, 0, NULL);
    if (!node) return NULL;
    // предполагаем, что node указывает на ely_str
    ely_str* s = (ely_str*)node;
    return *s ? ely_str_dup(*s) : NULL;
}

ely_int DictServer_get_int(DictServer* ds, char* path) {
    void* node = get_node(ds, path, 0, NULL);
    if (!node) return 0;
    // предполагаем int*
    int* i = (int*)node;
    return *i;
}

ely_bool DictServer_get_bool(DictServer* ds, char* path) {
    void* node = get_node(ds, path, 0, NULL);
    if (!node) return 0;
    int* b = (int*)node;
    return *b;
}

ely_double DictServer_get_double(DictServer* ds, char* path) {
    void* node = get_node(ds, path, 0, NULL);
    if (!node) return 0.0;
    double* d = (double*)node;
    return *d;
}

dict* DictServer_get_dict(DictServer* ds, char* path) {
    void* node = get_node(ds, path, 0, NULL);
    if (!node) return NULL;
    dict** d = (dict**)node;
    return *d;
}

arr* DictServer_get_array(DictServer* ds, char* path) {
    void* node = get_node(ds, path, 0, NULL);
    if (!node) return NULL;
    arr** a = (arr**)node;
    return *a;
}

// Установка значений (создаёт путь)
void DictServer_set_str(DictServer* ds, char* path, ely_str value) {
    void* node = get_node(ds, path, 1, NULL);
    if (!node) return;
    ely_str* dest = (ely_str*)node;
    if (*dest) ely_free(*dest);
    *dest = value ? ely_str_dup(value) : NULL;
}

void DictServer_set_int(DictServer* ds, char* path, ely_int value) {
    void* node = get_node(ds, path, 1, NULL);
    if (!node) return;
    int* dest = (int*)node;
    *dest = value;
}

void DictServer_set_bool(DictServer* ds, char* path, ely_bool value) {
    void* node = get_node(ds, path, 1, NULL);
    if (!node) return;
    int* dest = (int*)node;
    *dest = value;
}

void DictServer_set_double(DictServer* ds, char* path, ely_double value) {
    void* node = get_node(ds, path, 1, NULL);
    if (!node) return;
    double* dest = (double*)node;
    *dest = value;
}

void DictServer_set_dict(DictServer* ds, char* path, dict* value) {
    void* node = get_node(ds, path, 1, NULL);
    if (!node) return;
    dict** dest = (dict**)node;
    if (*dest) dict_free(*dest);
    // Создаём копию словаря (нужна функция dict_copy)
    // Пока просто сохраняем ссылку (осторожно!)
    *dest = value; // владение передаётся серверу? Лучше копировать.
    // Для упрощения пусть будет ссылка.
}

void DictServer_set_array(DictServer* ds, char* path, arr* value) {
    void* node = get_node(ds, path, 1, NULL);
    if (!node) return;
    arr** dest = (arr**)node;
    if (*dest) arr_free(*dest);
    *dest = value;
}

void DictServer_set_null(DictServer* ds, char* path) {
    void* node = get_node(ds, path, 1, NULL);
    if (!node) return;
    // Удаляем значение, установив NULL
    // Но в нашем представлении NULL – это особое значение.
    // Для простоты установим NULL в соответствующем месте.
    // Нужно определить, как хранить NULL. Можно хранить NULL-указатель.
    // В get_node, если значение NULL, оно вернётся как NULL.
    // При установке мы просто кладём NULL.
    // Однако в текущей реализации set_str и др. не поддерживают NULL.
    // Поэтому лучше выделить специальную функцию.
    // Пока оставим заглушку.
}

void DictServer_delete(DictServer* ds, char* path) {
    // Удаление узла по пути. Нужно найти родительский узел и удалить ключ/индекс.
    // Это сложнее. Пока не реализуем.
}

arr* DictServer_query(DictServer* ds, char* filter) {
    // Простой поиск – заглушка
    return arr_new(sizeof(void*));
}

void DictServer_free(DictServer* ds) {
    if (ds) {
        if (ds->path) ely_free(ds->path);
        if (ds->root) dict_free(ds->root);
        ely_free(ds);
    }
}

// ======================== Экспортируемые функции для модуля ========================
void* load(char* path) {
    DictServer* ds = new_dictserver(path);
    // Если файл существует, загрузить из него (можно реализовать позже)
    return ds;
}

void save(void* host, char* path) {
    DictServer* ds = (DictServer*)host;
    if (ds) {
        if (path) {
            if (ds->path) ely_free(ds->path);
            ds->path = ely_str_dup(path);
        }
        DictServer_save(ds);
    }
}

char* getStr(void* host, char* key) {
    return DictServer_get_str((DictServer*)host, key);
}

int getInt(void* host, char* key) {
    return DictServer_get_int((DictServer*)host, key);
}

int getBool(void* host, char* key) {
    return DictServer_get_bool((DictServer*)host, key);
}

double getDouble(void* host, char* key) {
    return DictServer_get_double((DictServer*)host, key);
}

void* getObj(void* host, char* key) {
    return DictServer_get_dict((DictServer*)host, key);
}

void setStr(void* host, char* key, char* value) {
    DictServer_set_str((DictServer*)host, key, value);
}

void setInt(void* host, char* key, int value) {
    DictServer_set_int((DictServer*)host, key, value);
}

void setBool(void* host, char* key, int value) {
    DictServer_set_bool((DictServer*)host, key, value);
}

void setDouble(void* host, char* key, double value) {
    DictServer_set_double((DictServer*)host, key, value);
}

void setObj(void* host, char* key, void* value) {
    DictServer_set_dict((DictServer*)host, key, (dict*)value);
}

void del(void* host, char* key) {
    DictServer_delete((DictServer*)host, key);
}

int has(void* host, char* key) {
    // Реализуйте DictServer_has, если нужно, или используйте dict_has_str
    return 0; // заглушка
}

arr* keys(void* host) {
    DictServer* ds = (DictServer*)host;
    if (!ds || !ds->root) return arr_new(sizeof(char*));
    return dict_keys_str(ds->root);
}

char* toJson(void* host) {
    DictServer* ds = (DictServer*)host;
    if (!ds || !ds->root) return ely_str_dup("null");
    return ely_dict_to_json(ds->root);
}

void* parse(char* json) {
    dict* d = ely_dictify(json);
    if (!d) return NULL;
    DictServer* ds = ely_alloc(sizeof(DictServer));
    ds->path = NULL;
    ds->root = d;
    return ds;
}

void freeDict(void* host) {
    DictServer_free((DictServer*)host);
}