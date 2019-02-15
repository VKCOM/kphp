@kphp_should_fail
<?php

/**
 * @kphp-template T $arg
 * @kphp-return T::data
 */
function f($arg)  {
    return 123;
}

f(10)->a = 10;

