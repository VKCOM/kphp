@ok
<?php

class BB {
    var $x = 20;

    public function foo() {
        var_dump("B::foo");
    }

    public function bar() {
        self::foo();
        $this->foo();
    }

    public function print_me() {
        var_dump("BB::me: ".$this->x);
    }
}

class B extends BB {
    var $y = 30;

    public function bar() {
        var_dump("B::bar");
        parent::bar();
    }

    public function print_me() {
        var_dump("B::me: ".$this->y);
    }
}

class D extends B {
    public function foo() {
        var_dump("D::foo");
        parent::foo();
        parent::print_me();
        BB::print_me();
    }
}

class D2 extends B {}

$call_bar = function(BB $b) {
    $b->bar();
};

$call_bar(new D2());
$call_bar(new D());
$call_bar(new B());
$call_bar(new BB());
