@ok
<?php

class Base {
    public $x;
}

class Derived extends Base {
    public function __construct() {
        $this->x = 20;
    }

    public function foo() {
        var_dump($this->x);
    }
}

$d = new Derived();
$d->foo();

