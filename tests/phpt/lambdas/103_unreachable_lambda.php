@kphp_should_fail
/Variable \$x has Unknown type/
<?php

/**
 * @return int
 */
function f() {
    return 10;
    array_map(function ($x) { return $x; }, [1, 2, 3]);
}

f();
