@kphp_should_fail
/in callF2\$n2<int, string>/
/Too many arguments in call to f2\(\), expected 2, have 5/
<?php

function f2(int $a, string $b) {
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$args
 */
function callF2(...$args) {
    f2(0, ...$args, ...$args);
}

callF2(1, 's');

