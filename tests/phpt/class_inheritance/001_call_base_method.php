@ok
<?php

class Base {
    public function foo() {
        var_dump("Base::foo");
    }
}

class Derived extends Base {
    public function __construct() {}

    public function bar() {
        $this->foo();
    }
}

$d = new Derived();
$d->foo();
