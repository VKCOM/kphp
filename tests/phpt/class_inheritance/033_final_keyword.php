@ok
<?php

class B {
    public function foo() {
        var_dump("B");
    }
}

class Base extends B {
    final public function foo() {
        var_dump("BASe");
    }

    public function bar() {
        var_dump("Base");
    }
}

final class Derived extends Base {
    final public function bar() {
        var_dump("derived");
    }
}


/** @var B */
$b = true ? new Derived() : (true ? new Base() : new B);
$b->foo();

if ($b instanceof Base) {
    $b->bar();
}
