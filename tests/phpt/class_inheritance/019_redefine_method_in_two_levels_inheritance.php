@ok
<?php

class B {
    public function foo() {
        var_dump("B");
    }
}

class D extends B {
    public function foo() {
        var_dump("D");
    }
}

class D2 extends D {
    public function foo() {
        var_dump("D2");
    }
}

/**@var B*/
$b = new B();
$b = new D2();
$b->foo();

