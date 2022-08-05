@kphp_should_fail
/g\/\*<AAA>\*\/\(null\)/
/Could not find class AAA/
<?php

/**
 * @kphp-generic T
 * @param T $arg
 */
function g($arg) {
}

g/*<AAA>*/(null);

