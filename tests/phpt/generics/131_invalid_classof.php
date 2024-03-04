@kphp_should_fail
/Expected ::class as a parameter, but got not a const string/
/Couldn't reify generic <T> for call/
<?php

class A {
    const NAME = 'A';
}

/**
 * @kphp-generic T
 * @param class-string<T> $class_name
 */
function printNameConst($class_name) {
    echo 'NAME=', $class_name::NAME, "\n";
}


printNameConst(classof(new A));
