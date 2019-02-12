<?php

/**
 * @kphp-lib-export
 */
function get_static_array() {
    static $x = [1, 2, 3, 4];
    return $x;
}
