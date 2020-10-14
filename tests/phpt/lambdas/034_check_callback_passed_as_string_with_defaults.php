@ok
<?php

/**
 * @kphp-required
 */
/**
 * @kphp-required
 * @param int $x
 * @return int
 */
function inc($x = 10) { return $x; }

function run() {
    $a = [1, 2];
    $new_a = array_map('inc', $a);
}

run();

