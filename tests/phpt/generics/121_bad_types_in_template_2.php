@kphp_should_fail
/pass mixed\[\] to argument \$rest of f<int>/
<?php

/**
 * @kphp-generic T
 * @param T $first
 * @param T ...$rest
 */
function f($first, ...$rest) {
}

f/*<int>*/(1, 1, 2, 2, [1]);
