<?php

class A {
    public $x = 20;
    public function foo() {
        $f = static function ($y) { return $y; };
        var_dump($f(10));
    }
}

$a = new A();
$a->foo();
