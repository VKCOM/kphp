@kphp_should_fail
/function f1/
/B1::f1<T> is a generic method, but parent A1::f1 is not/
/function f2/
/A2::f2<T> is a generic method, but child B2::f2 is not/
/@kphp-generic is not inherited, add it manually to child methods/
/function f3/
/Overriding non-abstract generic methods is forbidden: B3::f3<T>/
/function f4/
/I4::f4<T> is a generic method, but child B4::f4 is not/
<?php

abstract class A1 {
    abstract function f1();
}
class B1 extends A1 {
    /** @kphp-generic T */
    function f1() {}
}

abstract class A2 {
    /** @kphp-generic T */
    abstract function f2();
}
class B2 extends A2 {
    function f2() {}
}

class A3 {
    /** @kphp-generic T */
    function f3() {}
}
class B3 extends A3 {
    /** @kphp-generic T */
    function f3() {}
}

interface I4 {
    /** @kphp-generic T */
    function f4();
}
class B4 implements I4 {
    function f4() {}
}
