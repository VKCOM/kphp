@kphp_should_fail
/function f\(\$arg\) {}/
/Can't parse generic declaration <T> after ':'/
<?php

/**
 * @kphp-generic T: A|
 * @param T $arg
 */
function f($arg) {}

f(new A);
f(new B);
f(new C);
