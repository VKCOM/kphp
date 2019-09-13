<?php

abstract class A {
    public function bar() {
        var_dump("asdf");
    }
}

abstract class AClass extends A {
}

class B extends AClass {
    public function bar() {
        var_dump("zxcv");
    }
}

$b = new B();
$b->bar();
