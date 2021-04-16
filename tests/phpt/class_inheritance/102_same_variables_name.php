@kphp_should_fail
/You may not override field: `x`, in class: `D`/
<?php

class B {
    var $x = 20;

    public function foo() {
        var_dump($this->x);
    }
}

class D extends B {
    var $x = 1239;
}

/**@var B*/
$d = new B();
$d = new D();
$d->foo();
