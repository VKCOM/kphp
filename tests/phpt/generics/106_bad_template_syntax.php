@kphp_should_fail
/Parse file/
/function f\(\$arg\)/
/Duplicate generic <T> in declaration/
<?php

/**
 * @kphp-generic T, T
 * @param T $arg
 */
function f($arg) {}

f();

