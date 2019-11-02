@ok
<?php

require_once("polyfills.php");

class B {
    public function foo() {}
}

class D extends B {}

$d = new D();

/**@var B*/
$b = new B();
$b = $d;

var_dump($b === $d);
var_dump($b === new B());

