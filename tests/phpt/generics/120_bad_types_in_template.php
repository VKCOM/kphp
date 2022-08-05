@kphp_should_fail
/pass string to argument \$a of f<int>/
<?php

/**
 * @kphp-generic T
 * @param T $a
 */
function f($a) {}

f/*<int>*/('5');

