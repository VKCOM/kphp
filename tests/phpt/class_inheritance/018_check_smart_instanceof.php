@ok
<?php

class B {
    var $x = 20;

    public function foo() {
        var_dump((bool)$this);
        var_dump("B".$this->x);
    }
}

class E2 extends B {
    public function foo() {
        var_dump("E2");
    }
}

class E extends B {
}

/**@var B*/
$b = new E();
$b = new B();
$b->foo();

