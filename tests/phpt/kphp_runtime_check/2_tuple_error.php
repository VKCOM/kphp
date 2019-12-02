@kphp_should_fail
/Got unexpected type for @kphp-runtime-check: tuple/
<?php

/**
 * @kphp-runtime-check
 * @param $x1 tuple(string, int)
 */
function test_tuple_error($x1) { var_dump($x1); }

test_tuple_error("hello");
test_tuple_error(123);
