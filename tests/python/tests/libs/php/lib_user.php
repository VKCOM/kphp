<?php

require_once 'autoload.php';

#ifndef KPHP
function require_lib($lib_name) {
    return require_once($lib_name . "/php/index.php");
}
#endif

echo "lib_user: global run\n";

echo "lib_user: require_lib('lib_examples/example1');\n";
require_lib('lib_examples/example1');
example1_run();
require_lib('lib_examples/example1');
example1_run();

/**
 * @return int
 */
function call_example1_lib_functions() {
    echo "lib_user: minus1(5, 2): ", minus1(5, 2), "\n";
    echo "lib_user: foo('hello'): ", foo("hello"), "\n";
    return 1;
}

echo "lib_user: var_dump(get_str())\n";
var_dump(get_str());

echo "lib_user: Classes\A::some_function(): ", Classes\A::some_function(), "\n";

echo "lib_user: dev_global = call_example1_lib_functions();\n";
$dev_global = call_example1_lib_functions();

echo "lib_user: use_class_static_field();\n";
use_class_static_field();

echo "lib_user: use_class_static_function();\n";
use_class_static_function();

echo "lib_user: use_class_new();\n";
use_class_new();

echo "lib_user: pow2(): ", pow2(), "\n";

/**
 * @return int
 */
function use_other_lib() {
    require_lib('lib_examples/example2');
    return example2_sum(1, 2);
}

echo "lib_user: use_other_lib(): ", use_other_lib(), "\n";

echo "lib_user: get_static_array(): ";
var_dump(get_static_array());

var_dump(return_int_or_false(0));
var_dump(return_int_or_false_or_null(0));
var_dump(return_int_or_null(0));
var_dump(return_false_or_null(0));
var_dump(return_null(0));
var_dump(return_false(0));
