<?php

interface I {
    /**
     * @return string
     */
    public function foo();
}

class A implements I {
    public function foo() { return "asdf"; }
}

class B extends A {
    public function foo() { return "asdf"; }
}

class D extends B {
    public function foo() { return "asdf"; }
}

class D2 extends D {
    public function foo() { return "asdf"; }
}

/**@var A*/
$d = true ? new D2() : new A();
$d->foo();
