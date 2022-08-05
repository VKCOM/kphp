@kphp_should_fail
/it must not contain 'any' inside/
<?php

/**
 * @kphp-generic T
 * @param T $arr
 */
function f($arr) {}

f/*<array>*/([1,2,3]);
