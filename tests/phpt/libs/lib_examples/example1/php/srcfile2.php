<?php

/**
 * @kphp-lib-export
 * @return int[]
 */
function get_static_array() {
    static $x = [1, 2, 3, 4];
    return $x;
}
