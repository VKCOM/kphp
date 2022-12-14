@kphp_should_fail
/function vg\(\.\.\.\$args\)/
/@param for a variadic parameter is not an array/
<?php

/**
 * @param int $args
 */
function vg(...$args) {
    echo count($args);
}

vg(1, 's');
