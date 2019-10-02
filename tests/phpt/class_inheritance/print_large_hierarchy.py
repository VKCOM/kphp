#!/bin/python
def print_class(class_name, base_name=""):
    print(f"class {class_name} ", end="")
    if base_name:
        print(f"extends {base_name}")
    print("{")

    print("  function run($x) { return $x; }")
    print("}\n")


def print_n_depth_hierarchy(base_name, cnt_classes_in_depth):
    print_class(f"{base_name}_0")
    for i in range(1, cnt_classes_in_depth + 1):
        print_class(f"{base_name}_{i}", f"{base_name}_{i - 1}")


def print_n_breadth_hierarchy(base_name, cnt_classes_in_breadth):
    print_class(f"{base_name}_0")
    for i in range(1, cnt_classes_in_breadth + 1):
        print_class(f"{base_name}_{i}", f"{base_name}_0")


def print_n_depth_and_3_breadth_hierarchy(base_name, cnt_classes_in_depth):
    print_class(f"{base_name}_0")
    for i in range(1, cnt_classes_in_depth + 1):
        print_class(f"{base_name}_{i}", f"{base_name}_{i - 1}")
        for j in range(2):
            print_class(f"{base_name}_temp_{i}_{j}", f"{base_name}_{i}")


def create_main(base_name, cnt_classes_in_depth):
    def print_run(condition):
        run_name = f"run_{condition}_{base_name}";
        print(f"function {run_name}() {{")
        print(f"  /**@var {base_name}_0*/")
        print(f"  $x = {condition} ? new {base_name}_{cnt_classes_in_depth}() : new {base_name}_0();")
        print("  $y = 1;")

        print("  $start = microtime(true);")
        print("  for ($i = 0; $i < 1000000; ++ $i) $y ^= $x->run(42);")
        print("  var_dump(!defined('kphp') ? true : (microtime(true) - $start) < 0.06);")
        # print("  var_dump(microtime(true) - $start);")
        print("}\n")
        print(f"{run_name}();")

    print_run("false")
    print_run("true")


if __name__ == "__main__":

    print("@ok")
    print("<?php")
    print("# It's auto-generated file, please use `print_large_hierarchy.py`\n")

    base_name_depth = "A_depth"
    cnt_classes_in_depth = 7
    print_n_depth_and_3_breadth_hierarchy(base_name_depth, cnt_classes_in_depth)
    create_main(base_name_depth, cnt_classes_in_depth)

    base_name_breadth = "A_breadth"
    cnt_classes_in_breadth = 7
    print_n_breadth_hierarchy(base_name_breadth, cnt_classes_in_breadth)
    create_main(base_name_breadth, cnt_classes_in_breadth)
