@ok
<?php

class B {
    public function foo() {
        var_dump("B::foo");
    }

    public function bar() {
        var_dump("B::bar");
    }
}

class D extends B {
}

class D2 extends D {
    public function foo() {
        var_dump("Derived2::foo");
    }
}

class D3 extends D {
    public function bar() {
        var_dump("Derived3::bar");
    }
}

$call_foo_and_bar = function(B $instance) {
    $instance->foo();
    $instance->bar();
};

$call_foo_and_bar(new B());
$call_foo_and_bar(new D());
$call_foo_and_bar(new D2());
$call_foo_and_bar(new D3());

