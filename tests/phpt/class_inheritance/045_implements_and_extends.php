@ok
<?php

interface IB {
    public function foo();
}

class B {
    public function foo() { var_dump("B"); }
}

class D extends B implements IB {
    public function foo() { var_dump("D"); }
}

$d = new D();
$d->foo();

/**@var B*/
$b = $d;
$b->foo();
