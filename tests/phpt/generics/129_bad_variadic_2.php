@kphp_should_fail
/Variadic argument \.\.\.\$args1 used incorrectly in a variadic generic/
/Variadic argument \.\.\.\$args2 used incorrectly in a variadic generic/
<?php

function f2(int $a, string $b) {
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$args1
 */
function callF2_1(...$args1) {
    $copy = $args1;
    f2(...$copy);
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$args2
 */
function callF2_2(...$args2) {
    echo $args2[0];
}


callF2_1(1, 's');
callF2_2(1, 's');
