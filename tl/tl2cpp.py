# Это прототип TL парсерса, который сейчас полностью переписан на C++ и встроен в kphp (PHP/compiler/pipes/code-gen/tl2cpp.cpp)
# Парсер разрабатывался постепенно, поэтому требовалось выделять какие-то замкнутые подмножества комбинаторов, постепенно увеличивая покрытие с поддержкой новых конструкций
# Для этого использовались функции is_trivial() у TLType и TLCombinator. Они потенциально могут быть полезными
#!/usr/bin/env python3

from __future__ import print_function
import struct
from sys import argv
import sys
import ctypes
import os
from enum import Enum

debug_mode = True


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


class Reader:
    def __init__(self):
        self.data = open(argv[1], 'rb').read()
        self.pos = 0

    def get_int(self):
        self.check_pos(4)
        res = struct.unpack('i', self.data[self.pos:self.pos + 4])[0]
        self.pos += 4
        return res

    def get_uint(self):
        self.check_pos(4)
        res = struct.unpack('I', self.data[self.pos:self.pos + 4])[0]
        self.pos += 4
        return res

    def get_long(self):
        self.check_pos(8)
        res = struct.unpack('q', self.data[self.pos:self.pos + 8])[0]
        self.pos += 8
        return res

    def get_string(self):
        self.check_pos(4)
        len = struct.unpack('B', self.data[self.pos:self.pos + 1])[0]
        if len < 254:
            self.pos += 1
        else:
            len = struct.unpack('i', self.data[self.pos:self.pos + 1])[0]
            self.pos += 4
        self.check_pos(len)
        res = self.data[self.pos:self.pos + len]
        self.pos += len
        self.pos += (-self.pos & 3)
        self.check_pos(0)
        return res.decode("ascii")

    def check_pos(self, size):
        assert self.pos + size <= len(self.data)


NODE_TYPE_TYPE = 1
NODE_TYPE_NAT_CONST = 2
NODE_TYPE_VAR_TYPE = 3
NODE_TYPE_VAR_NUM = 4
NODE_TYPE_ARRAY = 5
ID_VAR_NUM = 0x70659eff
ID_VAR_TYPE = 0x2cecf817
ID_INT = 0xa8509bda
ID_LONG = 0x22076cba
ID_DOUBLE = 0x2210c154
ID_STRING = 0xb5286e24
ID_VECTOR = 0x1cb5c415
ID_DICTIONARY = 0x1f4c618f
ID_MAYBE_TRUE = 0x3f9c8ef8
ID_MAYBE_FALSE = 0x27930a7b
ID_BOOL_FALSE = 0xbc799737
ID_BOOL_TRUE = 0x997275b5
TYPE_ID_BOOL = 0x250be282
FLAG_OPT_VAR = (1 << 17)
FLAG_EXCL = (1 << 18)
FLAG_OPT_FIELD = (1 << 20)
FLAG_NOVAR = (1 << 21)
FLAG_DEFAULT_CONSTRUCTOR = (1 << 25)
FLAG_BARE = (1 << 0)
FLAG_NOCONS = (1 << 1)
FLAGS_MASK = ((1 << 16) - 1)
TLS_SCHEMA_V2 = 0x3a2f9be2
TLS_SCHEMA_V3 = 0xe4a8604b
TLS_TYPE = 0x12eb4386
TLS_COMBINATOR = 0x5c0a1ed5
TLS_COMBINATOR_LEFT_BUILTIN = 0xcd211f63
TLS_COMBINATOR_LEFT = 0x4c12c6d9
TLS_COMBINATOR_RIGHT_V2 = 0x2c064372
TLS_ARG_V2 = 0x29dfe61b
TLS_EXPR_TYPE = 0xecc9da78
TLS_NAT_CONST = 0xdcb49bd8
TLS_NAT_VAR = 0x4e8a14f0
TLS_TYPE_VAR = 0x0142ceae
TLS_ARRAY = 0xd9fb20de
TLS_TYPE_EXPR = 0xc1863d08

used_types = {}
used_ctors = {}

cells = []
cur_combinator = None

custom_impl_types = ["Bool", "Vector", "Maybe", "Dictionary", "DictionaryField", "IntKeyDictionary", "IntKeyDictionaryField", "LongKeyDictionary", "LongKeyDictionaryField", "Tuple"]

class TLType:
    def __init__(self, reader):
        self.id = reader.get_uint()
        self.name = reader.get_string()
        self.constructors_num = reader.get_int()
        self.constructors = []
        self.flags = reader.get_int()
        if self.name == "Maybe" or self.name == "Bool":
            self.flags |= FLAG_NOCONS
        self.arity = reader.get_int()
        reader.get_long()  # probably, should use it

    def __str__(self):
        return str(self.__dict__)

    def is_trivial(self):
        if self.name in custom_impl_types:
            used_types[self.id] = True
            return True
        if self.id in used_types and used_types[self.id] in [False, True]:
            return used_types[self.id]
        used_types[self.id] = -1
        for c in self.constructors:
            if not (c.id in used_ctors and used_ctors[c.id] == -1):
                if not c.is_trivial():
                    used_types[self.id] = False
                    return False
        used_types[self.id] = True
        return True

    def is_builtin(self):
        return self.name in ["Int", "Long", "Double", "String", "#", "Type", "False"]

    def is_dependent(self):
        for c in self.constructors:
            for arg in c.args:
                if isinstance(arg.type, TLTypeExpr) and types[arg.type.type_id].name == "Type":
                    return True
        return False


class TLTypeVar:
    def __init__(self, reader):
        self.var_num = reader.get_int()
        self.flags = reader.get_int()
        assert not (self.flags & (FLAG_NOVAR | FLAG_BARE));


class TLNatConst:
    def __init__(self, reader):
        self.num = reader.get_int()
        self.flags = FLAG_NOVAR


class TLNatVar:
    def __init__(self, reader):
        self.diff = reader.get_int()
        self.var_num = reader.get_int()
        self.flags = 0


class TLTypeExpr:
    def __init__(self, reader):
        self.type_id = reader.get_uint()
        assert self.type_id in types
        t = types[self.type_id]
        self.flags = reader.get_int() | FLAG_NOVAR
        assert t.arity == reader.get_int()
        self.child = []
        for i in range(t.arity):
            self.child.append(read_expr(reader))
            if not (self.child[-1].flags & FLAG_NOVAR):
                self.flags &= ~FLAG_NOVAR


class TLCell:
    def __init__(self, name, args, combinator):
        self.name = name
        self.args = args
        self.combinator = combinator


class TLTypeArray:
    def __init__(self, reader):
        global cur_combinator
        self.multiplicity = read_nat_expr(reader)
        num = reader.get_int()
        self.args = [TLArg(reader, i + 1) for i in range(num)]
        self.flags = FLAG_NOVAR
        self.cell_num = len(cells)
        for arg in self.args:
            assert not (arg.flags & FLAG_OPT_VAR)
        cells.append(TLCell("cell_" + str(self.cell_num), self.args, cur_combinator))
        for i in self.args:
            if not (i.flags & FLAG_NOVAR):
                self.flags &= ~FLAG_NOVAR


def read_nat_expr(reader):
    tp = reader.get_uint()
    if tp == TLS_NAT_CONST:
        return TLNatConst(reader)
    elif tp == TLS_NAT_VAR:
        return TLNatVar(reader)
    else:
        assert False


def read_expr(reader):
    tp = reader.get_uint()
    if tp == TLS_NAT_CONST:
        return read_nat_expr(reader)
    elif tp == TLS_EXPR_TYPE:
        return read_type_expr(reader)
    else:
        assert False


def read_type_expr(reader):
    tp = reader.get_uint()
    if tp == TLS_TYPE_VAR:
        return TLTypeVar(reader)
    elif tp == TLS_TYPE_EXPR:
        return TLTypeExpr(reader)
    elif tp == TLS_ARRAY:
        return TLTypeArray(reader)
    else:
        assert False


class TLArg:
    def __init__(self, reader, idx):
        global tl_schema_version
        schema_flag_opt_field = 2 << (tl_schema_version >= 3)
        schema_flag_has_vars = schema_flag_opt_field ^ 6
        assert reader.get_int() == TLS_ARG_V2

        self.name = reader.get_string()
        self.flags = reader.get_int()
        if self.flags & schema_flag_opt_field:
            self.flags &= ~schema_flag_opt_field
            self.flags |= FLAG_OPT_FIELD
        if self.flags & schema_flag_has_vars:
            self.flags &= ~schema_flag_has_vars
            self.var_num = reader.get_int()
        else:
            self.var_num = -1

        if self.flags & FLAG_OPT_FIELD:
            self.exist_var_num = reader.get_int()
            self.exist_var_bit = reader.get_int()
        else:
            self.exist_var_num = -1
            self.exist_var_bit = 0
        self.type = read_type_expr(reader)
        if self.var_num < 0 and self.exist_var_num < 0 and (self.type.flags & FLAG_NOVAR):
            self.flags |= FLAG_NOVAR
        self.idx = idx


class TLCombinator:
    def __init__(self, reader):
        global cur_combinator
        cur_combinator = self
        self.id = reader.get_uint()
        self.name = reader.get_string()
        self.type_id = reader.get_uint()
        left_type = reader.get_uint()
        if left_type == TLS_COMBINATOR_LEFT:
            num = reader.get_int()
            self.args = [TLArg(reader, i + 1) for i in range(num)]
        else:
            self.args = []
            assert left_type == TLS_COMBINATOR_LEFT_BUILTIN
        assert reader.get_int() == TLS_COMBINATOR_RIGHT_V2
        self.result = read_type_expr(reader)
        self.var_nums = {}
        for arg in self.args:
            if arg.var_num != -1:
                self.var_nums[arg.var_num] = arg
        cur_combinator = None

    def __str__(self):
        return str(self.__dict__)

    def get_first_arg(self):
        for arg in self.args:
            if not (arg.flags & FLAG_OPT_VAR):
                return arg
        return None

    def is_flat(self):
        if self.is_function():
            return False
        if types[self.type_id].constructors_num > 1:
            return False
        explicit_args_cnt = 0
        for arg in self.args:
            if arg.flags & FLAG_OPT_VAR:
                continue
            explicit_args_cnt += 1
        return explicit_args_cnt == 1

    def is_type_dependent(self):
        for arg in self.args:
            if isinstance(arg.type, TLTypeExpr) and types[arg.type.type_id].name == "Type":
                assert arg.flags & FLAG_OPT_VAR
                return True
        return False

    def is_constructor(self):
        return self in types[self.type_id].constructors

    def is_function(self):
        return not self.is_constructor()

    def is_trivial(self):
        if self.id in used_ctors and used_ctors[self.id] in [False, True]:
            return used_ctors[self.id]
        used_ctors[self.id] = -1

        def check_type_expr(type_expr):
            # fields_masks
            if isinstance(type_expr, TLNatVar) or isinstance(type_expr, TLNatConst):
                return True
            # type-dependence
            if isinstance(type_expr, TLTypeVar):
                return True
            # tl arrays, e.g. n * [...]
            if isinstance(type_expr, TLTypeArray):
                return True
            assert isinstance(type_expr, TLTypeExpr)
            if not (type_expr.type_id in used_types and used_types[type_expr.type_id] == -1):
                if not types[type_expr.type_id].is_trivial():
                    return False
            for child in type_expr.child:
                if not check_type_expr(child):
                    return False
            return True

        for arg in self.args:
            # ! sign maintenance
            if not check_type_expr(arg.type):  # or (arg.flags & FLAG_EXCL):
                used_ctors[self.id] = False
                return False
        if self.is_function():
            used_ctors[self.id] = check_type_expr(self.result)
        else:
            used_ctors[self.id] = True
        return used_ctors[self.id]


reader = Reader()


def get_schema_version():
    a = reader.get_int()
    if a == TLS_SCHEMA_V3:
        return 3
    if a == TLS_SCHEMA_V2:
        return 2
    assert False


def get_type():
    assert reader.get_int() == TLS_TYPE
    return TLType(reader)


types = {}
functions = {}


def get_types():
    count = reader.get_int()
    for i in range(count):
        t = get_type()
        types[t.id] = t


def get_combinator():
    assert reader.get_uint() == TLS_COMBINATOR
    return TLCombinator(reader)


def get_combinators():
    count = reader.get_int()
    for i in range(count):
        c = get_combinator()
        assert c.type_id in types
        types[c.type_id].constructors.append(c)


def get_functions():
    count = reader.get_int()
    for i in range(count):
        c = get_combinator()
        assert c.type_id in types
        functions[c.id] = c


tl_schema_version = get_schema_version()
reader.get_int()
reader.get_int()
get_types()
get_combinators()
get_functions()
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int_id = 0


class FileType(Enum):
    HEADER = 0
    CPP = 1


class FileWriter:
    def __init__(self, name):
        self.name = name
        self.deps = {}
        self.writer = None
        self.file_type = FileType.HEADER if name.endswith(".h") else FileType.CPP
        if self.file_type == FileType.CPP:
            self.deps["PHP/tl/auto/" + name.split(".")[0] + ".h"] = 1
        else:
            self.deps["PHP/tl/tl_init.h"] = 1

    def write_init(self):
        assert self.writer is None
        self.writer = open(PATH_TO_AUTO + self.name, "w")
        if self.file_type == FileType.HEADER:
            self.writer.write("#pragma once\n\n")
        for dep, n in self.deps.items():
            self.writer.write("#include \"%s\"\n" % dep)
        self.writer.write("\n")

    def add_dep(self, namespace):
        self.deps["PHP/tl/auto/" + namespace + ".h"] = 1


def get_name(x):
    return x.name.replace(".", "_")


def get_namespace(named):
    return named.name.split(".", 1)[0] if named.name.count(".") > 0 else "common"


def get_target_types():
    trivial = 7
    total = 0
    res = []
    for id, type in types.items():
        if type.name in custom_impl_types or type.name.startswith("tls.") or type.name.startswith("ping.") or type.name == "engine.Query":
            continue
        if type.name == "Int":
            global int_id
            int_id = id
        total += 1
        if type.is_trivial() and not type.is_builtin():
            res.append(type)
            trivial += 1
            is_type_dependent = False
            for c in type.constructors:
                cur_namespace = get_namespace(c)
                target_files[cur_namespace + ".h"] = FileWriter(cur_namespace + ".h")
                if not c.is_type_dependent():
                    target_files[cur_namespace + ".cpp"] = FileWriter(cur_namespace + ".cpp")
                else:
                    is_type_dependent = True
            type_namespace = get_namespace(type)
            target_files[type_namespace + ".h"] = FileWriter(type_namespace + ".h")
            if not is_type_dependent:
                target_files[type_namespace + ".cpp"] = FileWriter(type_namespace + ".cpp")
    print("Current stat:\n%d out of %d types (%.2f %%) can be automatically processed" % (
        trivial, total, trivial / total * 100))
    return res


def get_target_functions():
    trivial = 0
    total = 0
    res = []
    for id, f in functions.items():
        if f.name.startswith("tls.") or f.name.startswith("ping."):
            continue
        total += 1
        if f.is_trivial():
            res.append(f)
            trivial += 1
            cur_namespace = get_namespace(f)
            target_files[cur_namespace + ".h"] = FileWriter(cur_namespace + ".h")
            target_files[cur_namespace + ".cpp"] = FileWriter(cur_namespace + ".cpp")
    print("Current stat:\n%d out of %d functions (%.2f %%) can be automatically processed" % (
        trivial, total, trivial / total * 100))
    return res


def collect_file_dependencies():
    def check_type_tree(type_expr):
        if isinstance(type_expr, TLTypeArray):
            for arg in type_expr.args:
                check_type_tree(arg.type)
        if isinstance(type_expr, TLTypeExpr):
            cur_namespace = get_namespace(types[type_expr.type_id])
            if cur_namespace != outer_namespace:
                target_files[outer_namespace + ".h"].add_dep(cur_namespace)
            for child in type_expr.child:
                check_type_tree(child)

    for f in target_functions:
        outer_namespace = get_namespace(f)
        for arg in f.args:
            check_type_tree(arg.type)
        check_type_tree(f.result)

    for t in target_types:
        type_namespace = get_namespace(t)
        for c in t.constructors:
            outer_namespace = get_namespace(c)
            for arg in c.args:
                check_type_tree(arg.type)
            if type_namespace != outer_namespace:
                target_files[type_namespace + ".h"].add_dep(outer_namespace)


target_files = {}
target_types = get_target_types()
target_functions = get_target_functions()
collect_file_dependencies()

PATH_TO_ENGINE = argv[2] + "/"
PATH_TO_AUTO = PATH_TO_ENGINE + "PHP/tl/auto/"
PATH_TO_PHP_TL = PATH_TO_ENGINE + "PHP/tl/"

output = None


def get_file_writer(named, suf):
    filename = get_namespace(named) + suf
    return target_files[filename].writer


def gen_wrapper_decl(struct_name, constructor):
    is_constructor = struct_name.startswith("c_")
    is_type = struct_name.startswith("t_")
    type = types[constructor.type_id]

    global output
    output = get_file_writer(constructor if is_constructor else type, ".h")

    if constructor.is_type_dependent():
        typenames = []
        for arg in constructor.args:
            if arg.var_num != -1 and types[arg.type.type_id].name == "Type":
                assert arg.flags & FLAG_OPT_VAR
                typenames.append("typename T%d" % arg.var_num)
        output.write("template<%s>\n" % ", ".join(typenames))
    output.write("struct %s {\n" % struct_name)
    gen_wrapper_constructor_and_fields(struct_name, constructor)
    output.write("\tvoid store(const var& tl_object);\n")
    if constructor.is_flat():
        output.write("\tvar fetch();\n")
    else:
        output.write("\tarray<var> fetch();\n")
    output.write("};\n\n")


def gen_wrapper_def(struct_name, constructor):
    is_constructor = struct_name.startswith("c_")
    is_type = struct_name.startswith("t_")
    is_bare_type = is_type and struct_name.endswith("_$")
    type = types[constructor.type_id]

    global output
    if constructor.is_type_dependent():
        output = get_file_writer(constructor if is_constructor else type, ".h")
    else:
        output = get_file_writer(constructor if is_constructor else type, ".cpp")

    constructor_params_forward = list(map(lambda arg: arg.name,
                                          filter(lambda arg: arg.flags & FLAG_OPT_VAR, constructor.args)))
    template_params_forward = list(map(lambda arg: "T%d" % arg.var_num,
                                       filter(lambda arg: (arg.flags & FLAG_OPT_VAR) and types[arg.type.type_id].name == "Type", constructor.args)))

    typenames = list(map(lambda s: "typename " + s, template_params_forward))
    if len(typenames) > 0:
        template_decl = "template<%s>\n" % ", ".join(typenames)
        template_brackets = "<%s>" % ", ".join(template_params_forward)
    else:
        template_decl = ""
        template_brackets = ""

    full_struct_name = struct_name + template_brackets
    output.write(template_decl)
    output.write("void " + full_struct_name + "::store(const var& tl_object) {\n")
    if is_constructor:
        gen_store_def(constructor)
    elif is_type:
        gen_t_store(type, constructor_params_forward, template_brackets, is_bare_type)
    else:
        assert False
    output.write("}\n\n")

    output.write(template_decl)
    if constructor.is_flat():
        output.write("var ")
    else:
        output.write("array<var> ")
    output.write(full_struct_name + "::fetch() {\n")
    if is_constructor:
        gen_fetch_def(constructor)
    elif is_type:
        gen_t_fetch(type, constructor_params_forward, template_brackets, is_bare_type)
    else:
        assert False
    output.write("}\n\n")


def gen_t_wrapper_decl(t, is_bare_type):
    output.write("/* ~~~~~~~~~ %s ~~~~~~~~~ */\n\n" % t.name)
    struct_name = "t_" + get_name(t) + ("_$" if is_bare_type else "")
    gen_wrapper_decl(struct_name, t.constructors[0])


def gen_t_wrapper_def(t, is_bare_type):
    struct_name = "t_" + get_name(t) + ("_$" if is_bare_type else "")
    gen_wrapper_def(struct_name, t.constructors[0])


def gen_f_wrapper_decl(f):
    global output
    output = get_file_writer(f, ".h")

    output.write("/* ~~~~~~~~~ %s ~~~~~~~~~ */\n\n" % f.name)
    struct_name = "f_" + get_name(f)
    output.write("struct " + struct_name + " : tl_func_base {\n")
    for arg in f.args:
        if arg.flags & FLAG_OPT_VAR:
            output.write("\ttl_exclamation_fetch_wrapper %s;\n" % arg.name)
        elif arg.var_num != -1:
            output.write("\tint %s;\n" % arg.name)
    output.write("\tstatic std::shared_ptr<tl_func_base> store(const var& tl_object);\n")
    output.write("\tvar fetch();\n")
    output.write("};\n\n")


def gen_f_wrapper_def(f):
    global output
    output = get_file_writer(f, ".cpp")

    struct_name = "f_" + get_name(f)
    output.write("std::shared_ptr<tl_func_base> %s::store(const var& tl_object) {\n" % struct_name)
    if debug_mode:
        if not (f.name in ["memcache.get", "memcache.set"]):
            output.write("\tfprintf(stderr, \"~~~ store fun (auto gen) : " + get_name(f) + "\\n\");\n")
    output.write("\tstd::shared_ptr<%s> result_fetcher(new %s());\n" % (struct_name, struct_name))
    gen_store_def(f)
    output.write("\treturn result_fetcher;\n")
    output.write("}\n\n")
    output.write("var %s::fetch() {\n" % struct_name)
    if debug_mode:
        if not (f.name in ["memcache.get", "memcache.set"]):
            output.write("\tfprintf(stderr, \"~~~ fetch fun (auto gen) : " + get_name(f) + "\\n\");\n")
    gen_fetch_def(f)
    output.write("}\n\n")


def gen_cell(cell):
    global output
    output = target_files[get_namespace(cell.combinator) + ".h"].writer

    for arg in cell.args:
        assert not (arg.flags & (FLAG_OPT_VAR | FLAG_OPT_FIELD)) and isinstance(arg.type, TLTypeExpr)
    struct_name = cell.name
    output.write("//" + cell.combinator.name + "\n")
    if len(cell.args) == 1:
        output.write("using %s = %s;\n\n" % (struct_name, get_full_type(cell.args[0].type, "")))
        return
    output.write("struct " + struct_name + " {\n\n")
    output.write("void store(const var& tl_object) {\n")
    for arg in cell.args:
        gen_store_arg(arg, "")
    output.write("}\n\n")
    output.write("array<var> fetch() {\n")
    output.write("\tarray<var> result;\n")
    for arg in cell.args:
        gen_fetch_type_expr(arg.type, arg.name)
    output.write("\treturn result;\n")
    output.write("}\n\n")
    output.write("};\n\n")


def gen_integrating_hash_table():
    global output
    output = open(PATH_TO_PHP_TL + "tl_store_funs_table.cpp", "w")

    for name, file_writer in target_files.items():
        if file_writer.file_type == FileType.HEADER:
            output.write("#include \"%s\"\n" % ("PHP/tl/auto/" + name))
    output.write("\n")
    output.write("std::unordered_map<std::string, storer_ptr> auto_tl_storers_ht = {\n")
    for f in target_functions:
        pair = "{\"%s\", %s}" % (f.name, "&f_" + get_name(f) + "::store")
        if f == target_functions[0]:
            output.write("\t" + pair)
        else:
            output.write(",\n\t" + pair)
    output.write("\n};\n")


def get_full_type_expr_impl(type_expr, var_num_access):
    if isinstance(type_expr, TLNatVar):
        return None, var_num_access + cur_combinator.var_nums[type_expr.var_num].name
    if isinstance(type_expr, TLNatConst):
        return None, str(type_expr.num)
    if isinstance(type_expr, TLTypeVar):
        if cur_combinator.is_constructor():
            return "T" + str(type_expr.var_num), cur_combinator.var_nums[type_expr.var_num].name
        else:
            return "tl_exclamation_fetch_wrapper", cur_combinator.var_nums[type_expr.var_num].name
    if isinstance(type_expr, TLTypeArray):
        type = "tl_array<cell_%d>" % type_expr.cell_num
        val = type + "(%s, cell_%d())" % (get_full_value(type_expr.multiplicity, var_num_access), type_expr.cell_num)
        return type, val
    assert isinstance(type_expr, TLTypeExpr)
    type = types[type_expr.type_id]
    template_params = []
    constructor_params = []
    for child in type_expr.child:
        child_type_name, child_type_value = get_full_type_expr_impl(child, var_num_access)
        constructor_params.append(child_type_value)
        if child_type_name is not None:
            template_params.append(child_type_name)
    type_name = "t_" + (get_name(type) if type.name != "#" else get_name(types[int_id]))
    bare_suf = "_$" if bool(type_expr.flags & FLAG_BARE) or type.name == "#" else ""
    template = "<" + ", ".join(template_params) + ">" if len(template_params) > 0 else ""
    full_type_name = type_name + bare_suf + template
    full_type_value = full_type_name + "(" + ", ".join(constructor_params) + ")"
    return full_type_name, full_type_value


def get_full_value(type_expr, var_num_access):
    type, value = get_full_type_expr_impl(type_expr, var_num_access)
    return value


def get_full_type(type_expr, var_num_access):
    type, value = get_full_type_expr_impl(type_expr, var_num_access)
    return type


def get_fetcher_call(type_expr, var_num_access):
    return get_full_value(type_expr, var_num_access) + ".fetch()"


def gen_fetch_type_expr(type_expr, arg_name):
    output.write("\tresult.set_value(string(\"%s\"), %s);\n" % (arg_name, get_fetcher_call(type_expr, "")))


def gen_fetch_def(combinator):
    global cur_combinator
    cur_combinator = combinator
    if combinator.is_flat():
        output.write("\treturn %s;\n" % get_fetcher_call(combinator.get_first_arg().type, ""))
        return
    if combinator.is_function():
        output.write("\treturn %s;\n" % get_fetcher_call(combinator.result, ""))
        return
    output.write("\tarray<var> result;\n")
    for arg in combinator.args:
        if arg.flags & FLAG_OPT_VAR:
            continue
        if arg.flags & FLAG_OPT_FIELD:
            output.write("    if (%s & (1 << %d)) {\n" % (combinator.var_nums[arg.exist_var_num].name, arg.exist_var_bit))
        gen_fetch_type_expr(arg.type, arg.name)
        # запоминаем var_num для fields_mask
        if arg.var_num != -1 and types[arg.type.type_id].name == "#":
            output.write("\t%s = f$intval(result.get_value(string(\"%s\")));\n" % (combinator.var_nums[arg.var_num].name, arg.name))
            if debug_mode:
                output.write("\tfprintf(stderr, \"### fields_mask processing\\n\");\n")
        if arg.flags & FLAG_OPT_FIELD:
            output.write("    }\n")
    output.write("\treturn result;\n")


def get_storer_call(arg, var_num_access):
    return get_full_value(arg.type, var_num_access) + ".store(tl_arr_get(tl_object, string(\"%s\"), %d))" % (arg.name, arg.idx)


def gen_store_arg(arg, var_num_access):
    output.write("\t%s;\n" % get_storer_call(arg, var_num_access))


def gen_store_def(combinator):
    global cur_combinator
    cur_combinator = combinator
    var_num_access = "" if combinator.is_constructor() else "result_fetcher->"
    if combinator.is_flat():
        output.write("\t%s.store(tl_object);\n" % get_full_value(combinator.get_first_arg().type, var_num_access))
        return
    output.write("\t(void)tl_object;\n")
    if combinator.is_function():
        output.write("\tf$store_int(%s);\n" % hex(combinator.id))
    for arg in combinator.args:
        if arg.flags & FLAG_OPT_VAR:
            continue
        if arg.flags & FLAG_OPT_FIELD:
            # 2 случая:
            #      1) Зависимость от значения, которым параметризуется тип (имеет смысл только для конструкторов)
            #      2) Зависимость от значения, которое получаем в run-time (из явных аргументов)
            output.write(
                "    if (%s%s & (1 << %d)) {\n" % (var_num_access, combinator.var_nums[arg.exist_var_num].name, arg.exist_var_bit))
            # полиморфно обрабатываются, так как запоминаем все var_num'ы
        if not (arg.flags & FLAG_EXCL):
            gen_store_arg(arg, var_num_access)
        # запоминаем var_num для последующего использования
        if arg.flags & FLAG_EXCL:
            assert(isinstance(arg.type, TLTypeVar))
            cur_arg = "tl_arr_get(tl_object, string(\"%s\"), %d)" % (arg.name, arg.idx)
            cur_f_name = "tl_arr_get(%s, string(\"_\"), 0).as_string().c_str()" % cur_arg
            if debug_mode:
                output.write("\tfprintf(stderr, \"!!! exclamation processing...\\n\");\n")
            output.write("\tstd::string target_f_name = " + cur_f_name + ";\n")
            output.write("\t%s%s.fetcher = auto_tl_storers_ht[target_f_name](%s);\n" % (var_num_access, combinator.var_nums[arg.type.var_num].name, cur_arg))
        elif arg.var_num != -1 and types[arg.type.type_id].name == "#":
            output.write("\t%s%s = f$intval(tl_arr_get(tl_object, string(\"%s\"), %d));\n" % (
                var_num_access, combinator.var_nums[arg.var_num].name, arg.name, arg.idx))
            if debug_mode:
                output.write("\tfprintf(stderr, \"### fields_mask processing\\n\");\n")
        if arg.flags & FLAG_OPT_FIELD:
            output.write("    }\n")


def gen_wrapper_constructor_and_fields(name, constructor):
    constructor_params = []
    constructor_inits = []
    constructor_params_forward = []
    for arg in constructor.args:
        if arg.var_num != -1:  # для всех TLTypeVar и TLTypeArray arg.var_num == -1
            assert types[arg.type.type_id].name in ["#", "Type"]  # arg.var_num проставляется ТОЛЬКО для # или Type
            if types[arg.type.type_id].name == "#":
                output.write("\tint %s;\n" % arg.name)
                if arg.flags & FLAG_OPT_VAR:
                    constructor_params.append("int " + arg.name)
                    constructor_params_forward.append(arg.name)
                    constructor_inits.append("%s(%s)" % (arg.name, arg.name))
            elif types[arg.type.type_id].name == "Type":
                output.write("\tT%d %s;\n" % (arg.var_num, arg.name))
                assert arg.flags & FLAG_OPT_VAR
                constructor_params.append("const T%d& %s" % (arg.var_num, arg.name))
                constructor_params_forward.append(arg.name)
                constructor_inits.append("%s(%s)" % (arg.name, arg.name))
    if len(constructor_params) == 0:
        return []
    output.write("\texplicit %s(%s) : %s {}\n\n" % (name, ", ".join(constructor_params), ", ".join(constructor_inits)))
    return constructor_params_forward


def gen_t_store(type, constructor_params, template, is_bare_type):
    if type.constructors_num == 1:
        if is_bare_type:
            output.write("\tc_%s%s(%s).store(tl_object);\n" % (get_name(type.constructors[0]), template, ", ".join(constructor_params)))
        else:
            output.write("\tf$store_int(%s);\n" % hex(type.constructors[0].id))
            output.write("\tc_%s%s(%s).store(tl_object);\n" % (get_name(type.constructors[0]), template, ", ".join(constructor_params)))
        return
    output.write("\tstring constructor_name = f$strval(tl_arr_get(tl_object, string(\"_\"), 0));\n")
    default_constructor = type.constructors[len(type.constructors) - 1] if type.flags & FLAG_DEFAULT_CONSTRUCTOR else None
    for c in type.constructors:
        if c == type.constructors[0]:
            output.write("\tif ")
        else:
            output.write("\t} else if ")
        output.write("(constructor_name == string(\"%s\")) {\n" % c.name)
        if c != default_constructor:
            output.write("\t\tf$store_int(%s);\n" % hex(c.id))
        output.write("\t\tc_%s%s(%s).store(tl_object);\n" % (get_name(c), template, ", ".join(constructor_params)))
    output.write("\t} else {\n")
    output.write("\t\tdump_tl_array(tl_object);\n")
    output.write("\t\tphp_assert(false);\n")
    output.write("\t}\n")


def gen_t_fetch(type, constructor_parameters, template, is_bare_type):
    if is_bare_type:
        assert type.constructors_num == 1
        output.write("\treturn c_%s%s(%s).fetch();\n" % (get_name(type.constructors[0]), template, ", ".join(constructor_parameters)))
        return
    if type.constructors[0].is_flat():
        output.write("\tvar ")
    else:
        output.write("\tarray<var> ")
    output.write("result;\n")
    default_constructor = type.constructors[len(type.constructors) - 1] if type.flags & FLAG_DEFAULT_CONSTRUCTOR else None
    has_name = type.constructors_num > 1 and not (type.flags & FLAG_NOCONS)
    if default_constructor is not None:
        output.write("\tint pos = tl_parse_save_pos();\n")
    output.write("\tint magic = f$fetch_int();\n")
    output.write("\tswitch(magic) {\n")
    for c in type.constructors:
        if c == default_constructor:
            continue
        output.write("\t\tcase (int)%s: {\n" % hex(c.id))
        output.write("\t\t\tresult = c_%s%s(%s).fetch();\n" % (get_name(c), template, ", ".join(constructor_parameters)))
        if has_name:
            output.write("\t\t\tresult.set_value(string(\"_\"), string(\"%s\"));\n" % c.name)
        output.write("\t\t\tbreak;\n")
        output.write("\t\t}\n")
    output.write("\t\tdefault: {\n")
    if default_constructor is not None:
        output.write("\t\t\ttl_parse_restore_pos(pos);\n")
        output.write("\t\t\tresult = c_%s%s(%s).fetch();\n" % (get_name(default_constructor), template, ", ".join(constructor_parameters)))
        if has_name:
            output.write("\t\t\tresult.set_value(string(\"_\"), string(\"%s\"));\n" % default_constructor.name)
    else:
        output.write("\t\t\tfprintf(stderr, \"gotten magic: %d\\n\", magic);\n")
        output.write("\t\t\tphp_assert(false);\n")
    output.write("\t\t}\n")
    output.write("\t}\n")
    output.write("\treturn result;\n")


def check_constructor(c):
    # Проверяем, что порядок неявных аргументов зависимого типа совпадает с их порядком в конструкторе
    var_nums = []
    # flag = False
    for arg in c.args:
        # if arg.flags & FLAG_EXCL:
        #     flag = True
        if (arg.flags & FLAG_OPT_VAR) and arg.var_num != -1:
            var_nums.append(arg.var_num)
    params_order = []
    idx = 0
    for child in c.result.child:
        if isinstance(child, TLNatVar) or isinstance(child, TLNatConst) or isinstance(child, TLTypeVar):
            params_order.append(child.var_num)
        idx += 1
    if var_nums != params_order:
        print(var_nums, params_order)
        print("Strange tl-scheme here:", c.name)
    assert var_nums == params_order


# ~~~~~~~~~~~~ start of generation ~~~~~~~~~~~~

for s, file_writer in target_files.items():
    file_writer.write_init()

for t in target_types:
    for c in t.constructors:
        check_constructor(c)
        gen_wrapper_decl("c_" + get_name(c), c)

for t in target_types:
    gen_t_wrapper_decl(t, False)
    gen_t_wrapper_def(t, False)
    if t.constructors_num == 1:
        gen_t_wrapper_decl(t, True)
        gen_t_wrapper_def(t, True)

for cell in cells:
    if cell.combinator.is_constructor() and types[cell.combinator.type_id] in target_types or cell.combinator in target_functions:
        cur_combinator = cell.combinator
        gen_cell(cell)

for t in target_types:
    for c in t.constructors:
        cur_combinator = c
        gen_wrapper_def("c_" + get_name(c), c)

for f in target_functions:
    cur_combinator = f
    gen_f_wrapper_decl(f)
    gen_f_wrapper_def(f)

gen_integrating_hash_table()


def show_combinator(c):
    if c.is_constructor():
        print("@constructor:", c.name)
    else:
        print("@function:", c.name)
    idx = 0
    for arg in c.args:
        print("@arg#", idx, ":", arg.name)
        if isinstance(arg.type, TLTypeVar):
            print("TLTypeVar")
        elif isinstance(arg.type, TLTypeArray):
            print("TLTypeArray")
            print("multiplicity =", arg.type.multiplicity.var_num)
            for aarg in arg.type.args:
                print(aarg.type)
        # else:
        #     if arg.var_num != -1:
        #         type = types[arg.type.type_id]
        #         if not (type.name in ["#", "Type"]):
        #             print("###", type.name)

        print("var_num =", arg.var_num)
        print("flags:")
        if arg.flags & FLAG_OPT_VAR:
            print("\tFLAG_OPT_VAR")
        if arg.flags & FLAG_EXCL:
            print("\tFLAG_EXCL")
        if arg.flags & FLAG_OPT_FIELD:
            print("\tFLAG_OPT_FIELD", "exist_var_num =", arg.exist_var_num, "exist_var_bit =", arg.exist_var_bit)
        if arg.flags & FLAG_NOVAR:
            print("\tFLAG_NOVAR")
        if arg.flags & FLAG_DEFAULT_CONSTRUCTOR:
            print("\tFLAG_DEFAULT_CONSTRUCTOR")
        if arg.flags & FLAG_BARE:
            print("\tFLAG_BARE")
        if arg.flags & FLAG_NOCONS:
            print("\tFLAG_NOCONS")
        if arg.flags & FLAGS_MASK:
            print("\tFLAGS_MASK")
        idx += 1
    if isinstance(c.result, TLTypeVar):
        print("result: TLTypeVar")
    elif isinstance(c.result, TLTypeArray):
        print("result: TLTypeArray")
    else:
        print("result_children:")
        if len(c.result.child) == 0:
            print("empty")
        else:
            print("!non-empty!")
        for child in c.result.child:
            print("\t", child)
            if isinstance(child, TLNatVar):
                print("var_num =", child.var_num, "diff =", child.diff)
    print("~~~~~~~~~~~~~~~~~~~~~~~~")

# for id, f in types.items():
#     for c in f.constructors:
#         for arg in c.args:
#             if arg.name == "":
#                 print(f.name)
#
# for id, f in functions.items():
#     show_combinator(f)
# for id, type in types.items():
#     if type.name == "Maybe":
#         print(type.id)
#     x = 0
#     for c in type.constructors:
#         x += 0
#         show_combinator(c)
