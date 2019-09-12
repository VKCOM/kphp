@ok
<?php

trait Tr {
    public function foo() {
        var_dump("Tr");
    }
}

class A {
   use Tr;

    public function foo() {
        var_dump("A");
    }
}

class Derived extends A {
   use Tr;
   public $x = 20;
}

$run = function (A $a) { return $a->foo(); };

$run(new A());
$run(new Derived());

