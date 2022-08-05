@kphp_should_fail
/Can't find class: argument #2 is not a const string/
<?php

class B {
    function method() { echo "B method\n"; return 2; }
}

/**
 * @kphp-generic TName
 * @param class-string<TName> $class_name
 * @return TName
 */
function tplFWithLambda1($class_name) {
    // an error is missing use()
    $handler = function() {
        return instance_cast(new B, $class_name);
    };
    return $handler();
}


tplFWithLambda1(B::class)->method();

