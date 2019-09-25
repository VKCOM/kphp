@ok
<?php

/**
 * @kphp-infer
 * @param int ... $args
 */
function ints(...$args) {
  var_dump($args);
}

ints(1, 2);
