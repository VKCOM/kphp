@kphp_should_fail
/pass int\|array to argument \$x of f/
/declared as @param float\|string/
<?php

/** @param float|string $x */
function f($x) {}

function b() {
    /** @var int|array $y */
    $y = 42;
    f($y);
}

b();
