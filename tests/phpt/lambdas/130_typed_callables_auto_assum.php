@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/130_typed_callables_auto_assum.php:29/
/Declaration of L\\Avoid::__invoke must be compatible with version in class/
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
