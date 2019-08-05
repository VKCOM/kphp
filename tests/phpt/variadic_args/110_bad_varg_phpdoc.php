@kphp_should_fail
/Incorrect return type of function: ints/
<?php

/**
 * @kphp-infer
 * @param int $arg
 * @return int ...$
 */
function ints($arg) {
  return [$arg];
}

var_dump(ints(2));
