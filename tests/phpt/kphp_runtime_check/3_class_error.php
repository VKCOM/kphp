@kphp_should_fail
/Got unexpected type expr for @kphp-runtime-check: op_type_expr_class/
<?php

class A {
  public $x = 1;
}

/**
 * @kphp-runtime-check
 * @param $x1 A
 */
function test_class_error($x1) { var_dump(get_class($x1)); }

test_class_error("hello");
test_class_error(123);
