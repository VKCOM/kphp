#!/usr/bin/env python3

def generate_header():
    return """\
@ok
<?php
# THIS FILE IS GENERATED. DON'T EDIT IT. EDIT 457_eq2_gen.py
var_dump (array ("result" => array ("result" => 1, "_" => "resultTrue"), "_" => "_") === array ("result" => array ("result" => 2, "_" => "resultTrue"), "_" => "_"));

error_reporting (0);
var_dump (error_reporting());
"""


def generate_combined_arr(elements):
    return "$arr = [" + ", ".join(elements) + "];\n"


def generate_comparison(a, b, cmp_op, printable_a=None, printable_b=None):
    # printable versions for debug purpose because of php representation of any array is "Array"
    if printable_a is None:
        printable_a = a
    if printable_b is None:
        printable_b = b

    return f"echo 'line:'.__LINE__.' ' . gettype({a}).' '.gettype({b}).\" {printable_a}{cmp_op}{printable_b}: \"; " \
           f"var_dump ({a} {cmp_op} {b});\n"


def generate_comparison_with_mixed(elements, cmp_op):
    res = generate_combined_arr(elements)
    res += "foreach ($arr as $o) {\n"
    res += "  $po = json_encode($o);\n"
    for i, el in enumerate(elements):
        res += "  " + generate_comparison("$o", el, cmp_op, "$po", None)
        res += "  " + generate_comparison(el, "$o", cmp_op, None, "$po")
    res += "}\n"

    return res


def generate_cartesian_comparison(elements, cmp_op):
    res = ""
    for i, el_i in enumerate(elements):
        for j, el_j in enumerate(elements):
            res += generate_comparison(el_i, el_j, cmp_op)

    return res


def generate_cartesian_comparison_for_every_op(elements):
    ops = ["===", "==", "<", ">", "<=", ">=", "!=", "!=="]
    res = generate_combined_arr(elements)
    res += "foreach ($arr as $a) {\n"
    res += "  foreach ($arr as $b) {\n"
    res += "    $pa = json_encode($a); $pb = json_encode($b);\n"
    res += "    ".join([''] + [generate_comparison("$a", "$b", op, "$pa", "$pb") for op in ops])
    res += "  }\n"
    res += "}\n"

    return res


def generate_compare_test():
    test_body = generate_header()
    cmp_elements = ["21", "21.0", "0",
                    "'0'", "'1'", "'21'", "'199'", "''", "'21 apple'",
                    "true", "false", "null"]

    test_body += generate_cartesian_comparison(cmp_elements, "==") + "\n"
    # we skip comparisons arrays with other types as it will be a compilation error
    cmp_elements += ["array()", "array(0)", "[-183415123]", "[-51189706]"]
    test_body += generate_comparison_with_mixed(cmp_elements, "==") + "\n"
    test_body += generate_cartesian_comparison_for_every_op(cmp_elements) + "\n"

    cmp_elements = ["0", "1", "-1", "'0'", "'1'", "'-1'", "''", "null", "true", "false", "'10'", "-100", "1.5"]
    test_body += generate_cartesian_comparison_for_every_op(cmp_elements) + "\n"

    cmp_elements = ["'1e1'", "'     10e-0'", "'10e+0'", "'   1e1'", "'1e10'", "'10000000000'",
                    "'123456789012345678901234567890'", "'123456789012345678901234567891'", "'0'",
                    "'-0'", "10", "'.1e2'", "''", "false", "012"]
    test_body += generate_cartesian_comparison_for_every_op(cmp_elements) + "\n"

    open("457_eq2.php", "w").write(test_body)


if __name__ == '__main__':
    generate_compare_test()
