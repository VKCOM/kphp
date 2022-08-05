@kphp_should_fail
/Couldn't reify generic <T> for call: it's both A and B/
/Please, provide all generic types using syntax ABC::f\/\*<T>\*\/\(...\)/
<?php

class A {
    function method() { echo "A method\n"; return 1; }
}
class B {
    function method() { echo "B method\n"; return 2; }
}
class C {
    function method() { echo "C method\n"; return 3; }
}

class ABC {
/**
 * @kphp-generic T
 * @param T $a1
 * @param T $a2
 */
static function f($a1, $a2) {}
}

ABC::f(new A, new B);


