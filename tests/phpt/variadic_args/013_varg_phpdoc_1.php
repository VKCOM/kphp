@ok k2_skip
<?php

/**
 * @param int... $args
 */
function ints(...$args) {
  var_dump($args);
}

ints(1, 2);
