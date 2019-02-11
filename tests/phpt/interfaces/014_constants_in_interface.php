@ok
<?php

interface WithConstant {
    const b = "const";
    const a = 10;

    public function foo();
}

class A implements WithConstant {
    public function foo() { var_dump("A"); }
}

class B implements WithConstant {
    public function foo() { var_dump("A"); }
}

function test() {
    var_dump(WithConstant::a);
    var_dump(WithConstant::b);

    /** @var WithConstant $a */
    $a = new A;
    $a = new B;
    $a->foo();
}

test();
