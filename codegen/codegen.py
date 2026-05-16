import sys, os
from typing import List, Dict
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from parser import *
from codegen.classes_codegen import ClassCodeGen

class CppCodeGen(ClassCodeGen):
    def __init__(self, debug=False, is_module=False):
        super().__init__(debug, is_module)
        self.header_code: List[str] = []
        self.global_code: List[str] = []
        self.main_code: List[str] = []
        self.specializations: List[str] = []
        self.extern_functions: Dict[str, ExternFunction] = {}
        self._init_builtins()

    def _init_builtins(self):
        builtins = {
            'print':     ('ely_println', 'void', ['str']),
            'println':   ('ely_println', 'void', ['str']),
            'printOld':  ('ely_print',   'void', ['str']),
            'timeNow':     ('ely_time_now',      'more', []),
            'timeNowMs':   ('ely_time_now_ms',   'more', []),
            'timeDiff':    ('ely_time_diff',     'double', ['more', 'more']),
            'formatTime':  ('ely_format_time',   'str',   ['more', 'str']),
            'parseTime':   ('ely_parse_time',    'more',  ['str', 'str']),
            'sleep':       ('ely_sleep',         'void',  ['more']),
            'randInt':       ('ely_rand_int',        'int',  []),
            'randIntRange':  ('ely_rand_int_range',  'int',  ['int', 'int']),
            'randBool':      ('ely_rand_bool',       'bool', []),
            'srand':         ('ely_srand',           'void', ['uint']),
            'rand':          ('ely_rand',            'int',  []),
            'randDouble':    ('ely_rand_double',     'double', []),
            'fileWrite':    ('ely_file_write',    'int',  ['str', 'str']),
            'fileRead':     ('ely_file_read',     'str',  ['str']),
            'fileExists':   ('ely_file_exists',   'bool', ['str']),
            'fileReadAll':  ('ely_file_read_all', 'str',  ['str']),
            'fileClose':    ('ely_file_close',    'void', ['int']),
            'fileRemove':   ('ely_file_remove',   'int',  ['str']),
            'fileRename':   ('ely_file_rename',   'int',  ['str', 'str']),
            'pathJoin':        ('ely_path_join',        'str', ['str', 'str']),
            'pathBasename':    ('ely_path_basename',    'str', ['str']),
            'pathDirname':     ('ely_path_dirname',     'str', ['str']),
            'pathIsAbsolute':  ('ely_path_is_absolute', 'bool', ['str']),
            'loadLibrary':      ('ely_load_library',      'any',  ['str']),
            'getFunction':      ('ely_get_function',      'any',  ['any', 'str']),
            'callIntInt':       ('ely_call_int_int',      'int',  ['any', 'int']),
            'callDoubleDouble': ('ely_call_double_double','double',['any', 'double']),
            'callDoubleDoubleDouble': ('ely_call_double_double_double', 'double', ['any', 'double', 'double']),
            'callStrVoid':      ('ely_call_str_void',     'str',  ['any', 'str']),
            'closeLibrary':     ('ely_close_library',     'void', ['any']),
            'jsonify':  ('ely_dict_to_json', 'str', ['dict']),
            'dictify':  ('ely_dictify',      'object', ['str']),
            'keys': ('ely_dict_keys', 'arr<str>', ['dict']),
            'has':  ('ely_dict_has',  'bool',     ['dict', 'any']),
            'del':  ('ely_dict_del',  'void',     ['dict', 'any']),
            'len':      ('ely_str_len',       'int',   ['str']),
            'concat':   ('ely_str_concat',    'str',   ['str', 'str']),
            'dup':      ('ely_str_dup',       'str',   ['str']),
            'cmp':      ('ely_str_cmp',       'int',   ['str', 'str']),
            'substr':   ('ely_str_substr',    'str',   ['str', 'int', 'int']),
            'trim':     ('ely_str_trim',      'str',   ['str']),
            'replace':  ('ely_str_replace',   'str',   ['str', 'str', 'str']),
            'intToStr': ('ely_int_to_str',    'str',   ['int']),
            'strToInt': ('ely_str_to_int',    'int',   ['str']),
            'abs':      ('ely_abs_int',       'int',   ['int']),
            'absMore':  ('ely_abs_more',      'more',  ['more']),
            'fabs':     ('ely_fabs',          'double',['double']),
            'min':      ('ely_min_int',       'int',   ['int', 'int']),
            'max':      ('ely_max_int',       'int',   ['int', 'int']),
            'pow':      ('ely_pow',           'double',['double', 'double']),
            'sqrt':     ('ely_sqrt',          'double',['double']),
            'sin':      ('ely_sin',           'double',['double']),
            'cos':      ('ely_cos',           'double',['double']),
            'tan':      ('ely_tan',           'double',['double']),
            'typeof':  ('ely_typeof',         'str',   ['any']),
            'fields':  ('ely_value_get_fields', 'arr<str>', ['any']),
            'methods': ('ely_value_get_methods','arr<str>', ['any']),
            'isType': ('isType', 'bool', ['any', 'str']),
            'classInfoName': ('ely_get_class_info_name', 'str', ['str']),
        }
        self.set_builtins(builtins)

    def generate(self, program: Program) -> str:
        self.header_code = [
            'extern "C" {',
            '#include "ely_runtime.h"',
            '#include "ely_gc.h"',
            '}',
            '',
            '#include <stdio.h>',
            '#include <stdlib.h>',
            '#include <string.h>',
            '#include <setjmp.h>',
            '',
            'static jmp_buf __ex_buf;',
            'static ely_value* __ex_value = nullptr;',
            ''
        ]
        self.global_code = []
        self.main_code = []
        self.specializations = []
        self.type_aliases.clear()
        self.original_functions.clear()
        self.classes_ast.clear()
        self.namespaces.clear()
        self.imported_namespaces.clear()
        self.used_modules.clear()
        pending_usings = []

        for stmt in program.statements:
                if isinstance(stmt, NamespaceDeclaration):
                    old_ns = self.current_namespace
                    self.current_namespace = stmt.name
                    if stmt.name not in self.namespaces:
                        self.namespaces[stmt.name] = {}
                    for inner in stmt.body:
                        # Обрабатываем внутренние утверждения так же, как верхний уровень
                        if isinstance(inner, ClassDeclaration):
                            cls = inner
                            full_name = self.method_full_name(cls.name)   # с префиксом
                            self.classes_ast[full_name] = cls
                            all_methods = []
                            if cls.extends and cls.extends in self.classes_ast:
                                parent = self.classes_ast[cls.extends]
                                all_methods.extend(parent.all_methods)
                            all_methods.extend(cls.methods)
                            for prop in cls.properties:
                                if prop.getter: all_methods.append(prop.getter)
                                if prop.setter: all_methods.append(prop.setter)
                            cls.all_methods = all_methods
                            cls.static_methods = [m for m in cls.methods if m.modifier == 'static']
                            cls.static_fields  = [f for f in cls.fields  if f.modifier == 'static']
                            for method in cls.methods:
                                self.original_functions[f"{full_name}_{method.name}"] = method
                            self.original_functions[f"{full_name}_constructor"] = MethodDeclaration(
                                line=cls.line, col=cls.col,
                                return_type=full_name,
                                name=f"{full_name}_constructor",
                                parameters=[Parameter(type=f.type, name=f.name) for f in cls.wait_fields],
                                body=[], modifier='public'
                            )
                            self.namespaces[stmt.name][cls.name] = full_name
                        elif isinstance(inner, MethodDeclaration):
                            full_name = self.method_full_name(inner.name)
                            self.original_functions[full_name] = inner
                        elif isinstance(inner, ImplDeclaration):
                            if inner.class_name in self.classes_ast:
                                cls = self.classes_ast[inner.class_name]
                                for method in inner.methods:
                                    if method.name not in [m.name for m in cls.all_methods]:
                                        cls.methods.append(method)
                                        cls.all_methods.append(method)
                                        full_mname = f"{inner.class_name}_{method.name}"
                                        self.original_functions[full_mname] = method
                    self.current_namespace = old_ns
                    continue   # переходим к следующему stmt верхнего уровня

                # Обработка верхнего уровня (не namespace)
                if isinstance(stmt, ClassDeclaration):
                    self.classes_ast[stmt.name] = stmt
                    all_methods = []
                    if stmt.extends and stmt.extends in self.classes_ast:
                        parent = self.classes_ast[stmt.extends]
                        all_methods.extend(parent.all_methods)
                    all_methods.extend(stmt.methods)
                    for prop in stmt.properties:
                        if prop.getter: all_methods.append(prop.getter)
                        if prop.setter: all_methods.append(prop.setter)
                    stmt.all_methods = all_methods
                    stmt.static_methods = [m for m in stmt.methods if m.modifier == 'static']
                    stmt.static_fields  = [f for f in stmt.fields  if f.modifier == 'static']
                    for method in stmt.methods:
                        self.original_functions[f"{stmt.name}_{method.name}"] = method
                    self.original_functions[f"{stmt.name}_constructor"] = MethodDeclaration(
                        line=stmt.line, col=stmt.col,
                        return_type=stmt.name,
                        name=f"{stmt.name}_constructor",
                        parameters=[Parameter(type=f.type, name=f.name) for f in stmt.wait_fields],
                        body=[], modifier='public'
                    )
                elif isinstance(stmt, ImplDeclaration):
                    if stmt.class_name in self.classes_ast:
                        cls = self.classes_ast[stmt.class_name]
                        for method in stmt.methods:
                            if method.name not in [m.name for m in cls.all_methods]:
                                cls.methods.append(method)
                                cls.all_methods.append(method)
                                full_name = f"{stmt.class_name}_{method.name}"
                                self.original_functions[full_name] = method
                elif isinstance(stmt, MethodDeclaration):
                    self.original_functions[stmt.name] = stmt
                elif isinstance(stmt, TypeAlias):
                    self.type_aliases[stmt.name] = stmt.target_type
                elif isinstance(stmt, ExternFunction):
                    self.extern_functions[stmt.name] = stmt
                elif isinstance(stmt, GlobalCBlock):
                    self.global_code.append(stmt.code)
                elif isinstance(stmt, UsingDirective):
                    # Сохраняем для обработки после заполнения namespaces
                    pending_usings.append(stmt)
                elif isinstance(stmt, VariableDeclaration):
                    # Пока ничего, глобальные переменные будут обработаны позже
                    pass

            # ---- Теперь обрабатываем отложенные UsingDirective ----
        for using_stmt in pending_usings:
            self.used_modules.append(using_stmt.module)
            if using_stmt.module in self.namespaces:
                for short_name, full_name in self.namespaces[using_stmt.module].items():
                    self.imported_namespaces[short_name] = full_name

        # ---- Второй проход: регистрация содержимого namespace с префиксом ----
        for stmt in program.statements:
            if isinstance(stmt, NamespaceDeclaration):
                old_ns = self.current_namespace
                self.current_namespace = stmt.name
                if stmt.name not in self.namespaces:
                    self.namespaces[stmt.name] = {}
                for inner in stmt.body:
                    if isinstance(inner, ClassDeclaration):
                        cls = inner
                        full_name = self.method_full_name(cls.name)
                        self.classes_ast[full_name] = cls
                        all_methods = []
                        if cls.extends and cls.extends in self.classes_ast:
                            parent = self.classes_ast[cls.extends]
                            all_methods.extend(parent.all_methods)
                        all_methods.extend(cls.methods)
                        for prop in cls.properties:
                            if prop.getter: all_methods.append(prop.getter)
                            if prop.setter: all_methods.append(prop.setter)
                        cls.all_methods = all_methods
                        cls.static_methods = [m for m in cls.methods if m.modifier == 'static']
                        cls.static_fields  = [f for f in cls.fields  if f.modifier == 'static']
                        for method in cls.methods:
                            self.original_functions[f"{full_name}_{method.name}"] = method
                        self.original_functions[f"{full_name}_constructor"] = MethodDeclaration(
                            line=cls.line, col=cls.col,
                            return_type=full_name,
                            name=f"{full_name}_constructor",
                            parameters=[Parameter(type=f.type, name=f.name) for f in cls.wait_fields],
                            body=[],
                            modifier='public'
                        )
                        self.namespaces[stmt.name][cls.name] = full_name
                    elif isinstance(inner, MethodDeclaration):
                        full_name = self.method_full_name(inner.name)
                        self.original_functions[full_name] = inner
                    elif isinstance(inner, ImplDeclaration):
                        if inner.class_name in self.classes_ast:
                            cls = self.classes_ast[inner.class_name]
                            for method in inner.methods:
                                if method.name not in [m.name for m in cls.all_methods]:
                                    cls.methods.append(method)
                                    cls.all_methods.append(method)
                                    full_mname = f"{inner.class_name}_{method.name}"
                                    self.original_functions[full_mname] = method
                self.current_namespace = old_ns

        # ---- Генерация глобального кода в global_code ----
        old_code = self.code
        self.code = self.global_code
        try:
            self.global_code.append("void _global_init();")
            # Классы
            for cls in self.classes_ast.values():
                self.gen_class_full(cls)
                self.global_code.append('')
            # Статические поля
            self.gen_static_field_definitions()
            # Глобальные переменные
            for stmt in program.statements:
                if isinstance(stmt, VariableDeclaration):
                    self._gen_global_variable(stmt)
            # Глобальные функции (не методы классов)
            for stmt in program.statements:
                if isinstance(stmt, MethodDeclaration) and stmt.name not in [c.name for c in self.classes_ast.values()]:
                    self.current_class_name = None
                    self._gen_one_function(stmt)
            # Класс-инфо
            for cls in self.classes_ast.values():
                self.gen_class_info(cls)
            # Реестр классов
            self.global_code.append("static ely_class_info* class_registry[] = {")
            for cls in self.classes_ast.values():
                self.global_code.append(f"    &{cls.name}_class_info,")
            self.global_code.append("    nullptr")
            self.global_code.append("};")
            self.global_code.append("""
    ely_class_info* ely_get_class_info(const char* name) {
        for (int i = 0; class_registry[i] != nullptr; ++i)
            if (strcmp(class_registry[i]->name, name) == 0)
                return class_registry[i];
        return nullptr;
    }
    """)
            self.global_code.append("void _global_init() {")
            self.global_code.append("    // статические поля уже инициализированы")
            self.global_code.append("}")
        finally:
            self.code = old_code

        # main (если нет своего)
        if 'main' not in self.original_functions:
            self.main_code.append("int main() {")
            self.main_code.append("    gc_init();")
            self.main_code.append("    _global_init();")
            self.main_code.append("    gc_set_enabled(0);")
            self.main_code.append("    return 0;")
            self.main_code.append("}")

        return "\n".join(self.header_code + self.global_code + self.specializations + self.main_code)

    def _gen_one_function(self, method: MethodDeclaration):
        """Генерирует одну функцию и добавляет её в self.main_code."""
        old_main = self.main_code
        self.main_code = []          # временный буфер для тела функции
        # Перед генерацией очищаем method_code, чтобы gen_function записал туда код
        saved_method = self.method_code
        self.method_code = []
        self.gen_function(method)
        # Тело функции теперь находится в self.method_code
        self.main_code.extend(self.method_code)
        self.method_code = saved_method
        # Добавляем в общий main_code
        old_main.extend(self.main_code)
        self.main_code = old_main

    def _gen_global_variable(self, node: VariableDeclaration):
        resolved = self.resolve_type_alias(node.type)
        ctype = self.type_to_cpp(resolved)
        self.global_code.append(f"{ctype} {node.name};")
        self.global_types[node.name] = resolved
        if node.initializer:
            self.global_vars_to_init.append((node.name, resolved, node.initializer))