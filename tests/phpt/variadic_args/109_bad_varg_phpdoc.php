@kphp_should_fail
/Variable name expected/
<?php

/**
 * @param int ...$ $args
 */
function ints(...$args) {
  var_dump($args);
}

ints(1, 2);
