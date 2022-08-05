@kphp_should_fail
/'@kphp-generic \$arg' is an invalid syntax/
/@kphp-generic T/
/@param T \$arg/
<?php

/**
 * @kphp-generic $arg
 */
function f($arg) {
}

f(3);
