@ok
<?php

class B {
    public function foo() {
        var_dump("B::foo");
    }
}

class D extends B {
    public function foo() {
        var_dump("D::foo");
    }
}

class D2 extends B {
    public function foo() {
        var_dump("D2::foo");
    }
}

/** @var B */
$d = new B();
$d->foo();

$d = new D();
$d->foo();

$d = new D2();
$d->foo();

