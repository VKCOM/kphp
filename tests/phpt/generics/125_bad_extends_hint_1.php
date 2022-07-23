@kphp_should_fail
/Could not find class SomeI2/
/Could not parse generic T extends after 'T:'/
<?php

interface SomeI { function f(); }

class A implements SomeI { function f() { echo "A f\n"; } }
class B implements SomeI { function f() { echo "B f\n"; } }

/**
 * @kphp-generic T: SomeI2
 * @param T $o
 */
function tplOne($o) {
    $o->f();
}

tplOne(new A);
tplOne(new B);
