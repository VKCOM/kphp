@kphp_should_fail
/Parse file/
/tuple\(/
/Could not parse generic instantiation comment: unexpected end/
<?php

/**
 * @kphp-generic T
 * @param T $arg
 */
function f($arg) {}

f/*<tuple(>*/();

