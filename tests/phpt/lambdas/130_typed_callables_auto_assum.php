@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass B to argument \$arg0 of L\\Avoid::__invoke/
/assign string to A::\$a/
/but it's declared as @var int/
/pass A to argument \$b of anonymous\(...\)/
/but it's declared as @param B/
<?php


class A {
    /** @var int */
    var $a = 0;
}

class B {
    /** @var int */
    var $b = 0;
}

/** @param callable(A):void $cb */
function r($cb) {
    $cb(new A);
    $cb(new B);
}

// here $a is auto-detected that of class A
r(function($a) {
    $a->a = '3';
});
// here is also an error because an interface __invoke function passes A to B
r(function(B $b) {
    $b->b = 0;
});
