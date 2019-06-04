@ok
<?php

class A {
    public function foo() {}
}

class B {
    public function foo() {}
}

var_dump(new A() === new A());

$a1 = new A();
$a2 = $a1;
var_dump($a1 === $a2);

$b = new B();
var_dump($b == $a1);
var_dump($b === $a1);
