@kphp_should_fail
/f\(new C\);/
/Calculated generic <T> = C breaks condition 'T:A\|B'/
/It does not fit any of specified conditions/
<?php


class A {}
class B {}
class C {}

/**
 * @kphp-generic T: A|B
 * @param T $arg
 */
function f($arg) {}

f(new A);
f(new B);
f(new C);
