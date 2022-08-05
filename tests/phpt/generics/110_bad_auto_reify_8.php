@kphp_should_fail
/f\(\$a\);/
/Please, provide all generic types using syntax f\/\*<T>\*\/\(\.\.\.\)/
<?php

/**
 * @kphp-generic T
 * @param T $in
 */
function f($in) {}

function demo() {
    /** @var ?int $another */

    $a = null;
    f($a);  // can't be reified, because phpdoc goes after
    /** @var ?int $a */
}

demo();
