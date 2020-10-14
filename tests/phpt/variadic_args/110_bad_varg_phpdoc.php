@kphp_should_fail
/Variable name expected/
<?php

/**
 * @param int $arg
 * @return int ...$
 */
function ints($arg) {
  return [$arg];
}

var_dump(ints(2));
