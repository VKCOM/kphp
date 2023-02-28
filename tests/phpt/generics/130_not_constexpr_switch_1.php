@kphp_should_fail
/in fact1<A>/
/Method bMethod\(\) not found in class A/
/in fact1<B>/
/Method aMethod\(\) not found in class B/
/in fact2<A>/
/in fact2<B>/
/\$obj is assumed to be both A and B/
/in fact3<A>/
/\$obj is assumed to be both A3 and B3/
<?php

class A { function __construct(int $x) {} function aMethod() {} }
class B { function bMethod() {} }
class A3 { function __construct(int $x) {} function aMethod() {} }
class B3 { function bMethod() {} }

/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return T
 */
function fact1(string $cn) {
    /** @var T $obj */
    $copy = $cn;        // becomes not constexpr
    switch($copy) {
    case A::class:
        $obj = new A(10);
        $obj->aMethod();
        break;
    case B::class:
        $obj = new B();
        $obj->bMethod();
        break;
    }
    return $obj;
}

fact1(A::class);
fact1(B::class);


/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return T
 */
function fact2(string $cn) {
    $copy = $cn;        // becomes not constexpr
    switch($copy) {
    case A::class:
        $obj = new A(10);
        $obj->aMethod();
        break;
    case B::class:
        $obj = new B();
        $obj->bMethod();
        break;
    }
    return $obj;
}

fact2(A::class);
fact2(B::class);


/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return T
 */
function fact3(string $cn) {
    switch($cn) {
    case A::class:
        $obj = new A3(10);
        $obj->aMethod();
    case B::class:
        $obj = new B3();
        $obj->bMethod();
        break;
    }
    return $obj;
}

fact3(A::class);
