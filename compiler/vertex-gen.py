#!/usr/bin/python3
import argparse
import json
import shutil
import sys
import jsonschema
from pathlib import Path

def clear_dir():
    if DIR.exists():
        shutil.rmtree(str(DIR))
    DIR.mkdir()


def open_file(name, once=True):
    f = (DIR / name).open('w')
    f.write("/* THIS FILE IS GENERATED. DON'T EDIT IT. EDIT vertex-desc.json.*/\n")
    if once:
        f.write("#pragma once\n")
    return f


def get_schema_properties(schema):
    return schema["definitions"]["operation_property_dict"]["properties"]


def output_one_enum(f, name, values):
    f.write("enum {name} {{\n".format(name=name))

    for val in values:
        f.write("  " + val + ",\n")

    f.write("};\n")


def output_enums(data, schema):
    with open_file("vertex-types.h") as f:
        output_one_enum(f, "Operation", [i["name"] for i in data] + ["Operation_size"])

        props = get_schema_properties(schema)
        for i in props:
            if "enum" in props[i]:
                f.write("\n")
                name = "opp_%s_t" % i
                if "title" in props[i]:
                    name = props[i]["title"]
                output_one_enum(f, name, props[i]["enum"])


def output_include_directive(f, name):
    if name == "meta_op_base":
        f.write('#include "compiler/vertex-meta_op_base.h"\n')
    else:
        f.write('#include "%s/vertex-%s.h"\n' % (REL_DIR, name))


def output_class_header(f, base_name, name, final_specifier="", comment=""):
    comment_line = "\n// " + comment if comment else ""
    f.write("""{comment_line}
template<>
class vertex_inner<{name}> {final_specifier}: public vertex_inner<{base_name}> {{
public:""".format(comment_line=comment_line, name=name, base_name=base_name, final_specifier=final_specifier))


def get_string_extra():
    return """
  virtual const string &get_string() const override { return str_val; }
  virtual void set_string(string s) override { str_val = std::move(s); }
  virtual bool has_get_string() const override { return true; }
"""

def output_extras(f, type_data):
    if "extras" in type_data:
        type_data.setdefault("extra_fields", {})

        do_with_extras = {
            "string": ("str_val", "std::string", get_string_extra),
        }

        for extra_name in type_data["extras"]:
            assert extra_name in do_with_extras
            (var_name, type, fun_get_extra) = do_with_extras[extra_name]

            type_data["extra_fields"][var_name] = {'type': type}
            f.write(fun_get_extra())


def output_one_extra_field(f, name, desc):
    f.write('  ' + desc["type"] + ' ' + name)
    if "default" in desc:
        f.write(" = " + str(desc["default"]))
    f.write(";\n")


def output_extra_fields(f, type_data):
    if "extra_fields" in type_data:
        f.write("private:\n")
        extra_fields = type_data["extra_fields"]
        for extra_field in extra_fields:
            if extra_field[-1] == '_':
                output_one_extra_field(f, extra_field, extra_fields[extra_field])

        f.write("public:\n")
        for extra_field in extra_fields:
            if extra_field[-1:] != '_':
                output_one_extra_field(f, extra_field, extra_fields[extra_field])


def parents(data, name):
    while True:
        for type in data:
            if type["name"] != name:
                continue
            yield type
            if "base_name" in type:
                name = type["base_name"]
            else:
                return


def is_varg(data, name):
    for type in parents(data, name):
        if "ranges" in type:
            return True
    return False


def get_argument(data, name, id):
    for type in parents(data, name):
        if "sons" in type:
            for son_name, son in type["sons"].items():
                if isinstance(son, int) and son == id:
                    return son_name, {"id": son}
                if isinstance(son, dict) and son["id"] == id:
                    return son_name, son
    return None, None


def output_create_function(f, data, name):
    f.write("""
  template<typename... Args>
  static vertex_inner<{name}> *create_vararg(Args&&... args) {{
    auto v = raw_create_vertex_inner<{name}>(get_children_size(std::forward<Args>(args)...));
    v->set_children(0, std::forward<Args>(args)...);
    return v;
  }}
""".format(name=name))
    if is_varg(data, name):
        f.write("""
  template<typename... Args>
  static vertex_inner<{name}> *create(Args&&... args) {{
    return create_vararg(std::forward<Args>(args)...);
  }}
""".format(name=name))
        return
    args_decl = []
    args_call = []

    def write_func():
        f.write("""
  static vertex_inner<{name}> *create({args_decl}) {{
    return create_vararg({args_call});
  }}
""".format(name=name, args_decl=', '.join(args_decl), args_call=', '.join(args_call)))

    id = 0
    while True:
        arg_name, arg = get_argument(data, name, id)
        if arg is None:
            break
        if "optional" in arg:
            write_func()
        if "type" not in arg:
            arg_type = 'const VertexPtr&'
        else:
            arg_type = 'const VertexAdaptor<' + arg["type"] + '>&'
        args_decl.append(arg_type + ' ' + arg_name)
        args_call.append(arg_name)
        id += 1
    write_func()


def output_props_dictionary(f, schema, props, cnt_spaces):
    for prop_name in props:
        prop_value = props[prop_name]
        if prop_name == "op_str" or get_schema_properties(schema)[prop_name].get("type", "") == "string":
            prop_value = '"' + prop_value + '"'

        spaces = cnt_spaces * " "
        f.write(spaces)
        f.write("p->{prop_name} = {prop_value};\n".format(prop_name=prop_name, prop_value=prop_value))


def output_props(f, type_data, schema):
    if "props" not in type_data:
        type_data["props"] = {}
    f.write("  static void init_properties(OpProperties *p) {\n")
    f.write("    vertex_inner<{name}>::init_properties(p);\n".format(name=type_data["base_name"]))

    type_data["props"].update({"op_str": type_data["name"]})
    output_props_dictionary(f, schema, type_data["props"], 4)

    f.write("  }\n")


def output_sons(f, type_data):
    if "sons" in type_data:
        f.write("\n")
        for son_name in type_data["sons"]:
            son_properties = type_data["sons"][son_name]
            if isinstance(son_properties, int):
                son_properties = {"id": son_properties}

            son_id = son_properties["id"] if "id" in son_properties else son_properties
            optional = "optional" in son_properties
            virtual = "virtual" if "virtual" in son_properties else ""
            override = "override" if "override" in son_properties else ""
            type = son_properties["type"] if "type" in son_properties else None

            if son_id < 0:
                son_id = "size() - " + str(-son_id)
            else:
                son_id = str(son_id)

            if optional:
                f.write("  {virtual} bool has_{son_name}() const {override} {{ return check_range({son_id}); }}\n"
                        .format(virtual=virtual, son_name=son_name, son_id=son_id, override=override))

            if type is not None:
                f.write("  {virtual} VertexAdaptor<{type}> {son_name}() const {override} {{ return ith({son_id}).as<{type}>(); }}\n"
                        .format(virtual=virtual, son_name=son_name, son_id=son_id, override=override, type=type))
                son_name += "_ref"
            f.write("  {virtual} VertexPtr &{son_name}() {override} {{ return ith({son_id}); }}\n"
                    .format(virtual=virtual, son_name=son_name, son_id=son_id, override=override))
            f.write("  {virtual} const VertexPtr &{son_name}() const {override} {{ return ith({son_id}); }}\n"
                    .format(virtual=virtual, son_name=son_name, son_id=son_id, override=override))


def output_aliases(f, type_data):
    if "alias" in type_data:
        f.write("\n")
        for i in type_data["alias"]:
            f.write("  VertexPtr &%s() { return %s(); }\n" % (i, type_data["alias"][i]))
            f.write("  const VertexPtr &%s() const { return %s(); }\n" % (i, type_data["alias"][i]))


def output_ranges(f, type_data):
    def convert_range(value, zero):
        if value > 0:
            return "begin() + %d" % value
        elif value < 0:
            return "end() - %d" % (-value)

        return zero

    if "ranges" in type_data:
        f.write("\n")
        for range_name in type_data["ranges"]:
            from_r = convert_range(type_data["ranges"][range_name][0], "begin()")
            to_r = convert_range(type_data["ranges"][range_name][1], "end()")

            f.write("  VertexRange %s() { return VertexRange(%s, %s); }\n" % (range_name, from_r, to_r))
            f.write("  VertexConstRange %s() const { return VertexConstRange(%s, %s); }\n" % (range_name, from_r, to_r))


def output_class_footer(f):
    f.write("};\n")


def output_vertex_type(type_data, data, schema):
    if "base_name" not in type_data: return

    (name, base_name) = (type_data["name"], type_data["base_name"])

    with open_file("vertex-" + name + '.h') as f:
        output_include_directive(f, base_name)

        if "sons" in type_data:
            for son_name, son_properties in type_data["sons"].items():
                if isinstance(son_properties, int):
                    continue
                type_prop = son_properties["type"] if "type" in son_properties else None
                if type_prop:
                    output_include_directive(f, type_prop)

        final_specifier = "final"
        for v in data:
            if "base_name" in v and type_data["name"] == v["base_name"]:
                final_specifier = ""
                break
        comment = type_data.get("comment", "")
        output_class_header(f, base_name, name, final_specifier, comment)

        output_extras(f, type_data)
        output_extra_fields(f, type_data)

        output_create_function(f, data, name)
        output_props(f, type_data, schema)

        output_sons(f, type_data)
        output_aliases(f, type_data)
        output_ranges(f, type_data)

        output_class_footer(f)


def output_all(data):
    with open_file("vertex-all.h") as f:
        for vertex in data:
            output_include_directive(f, vertex["name"])


def output_foreach_op(data):
    with open_file("foreach-op.h", once=False) as f:
        for vertex in data:
            if vertex["name"] != "op_err":
                f.write('FOREACH_OP(%s)\n' % vertex["name"])

        f.write("#undef FOREACH_OP\n")


def check_is_base(base, derived, data):
    while True:
        if derived["name"] == base["name"]:
            return True
        if "base_name" not in derived:
            return False
        for i in data:
            if i["name"] == derived["base_name"]:
                derived = i
                break

def output_vertex_is_base_of(data):
    with open_file("is-base-of.h") as f:
        f.write('#include "vertex-types.h"\n')
        f.write("constexpr bool op_type_is_base_of_data[Operation_size * Operation_size] = {\n")
        for base in data:
            for derived in data:
                if check_is_base(base, derived, data):
                    f.write("true, ")
                else:
                    f.write("false, ")
            f.write("\n")
        f.write("};\n")
        f.write('''
constexpr bool op_type_is_base_of(Operation Base, Operation Derived) {
  // gcc doesn't support indexing of array by two enums in constexpr
  return op_type_is_base_of_data[Base * Operation_size + Derived];
}
''')


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--auto', required=True, help='path to auto directory')
    parser.add_argument('--config', required=True, help='path to config.json')
    args = parser.parse_args()

    REL_DIR = Path(args.auto) / 'compiler' / 'vertex'
    DIR = Path(__file__).resolve().parents[2] / REL_DIR

    print(DIR)
    with open(args.config) as f:
        data = json.load(f)

    with open(args.config.replace(".json", ".config.json")) as f:
        schema = json.load(f)

    jsonschema.validators.Draft4Validator.check_schema(schema)
    jsonschema.validate(data, schema)

    clear_dir()
    output_enums(data, schema)

    for vertex in data:
        output_vertex_type(vertex, data, schema)

    output_all(data)
    output_vertex_is_base_of(data)
    output_foreach_op(data)
