<?php

/**
 * @kphp-lib-export
 * @param int $x
 * @param int $y
 * @return int
 */
function example2_sum($x, $y) {
    return $x + $y;
}

/**
 * @kphp-lib-export
 * @param mixed $x
 */
function set_value($x) {
    global $global_x;
    $global_x = $x;
}

/**
 * @kphp-lib-export
 * @return mixed
 */
function get_value() {
    global $global_x;
    return $global_x;
}

