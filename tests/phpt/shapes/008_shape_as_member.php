@ok
<?php
require_once 'kphp_tester_include.php';

function demo(Classes\Z1 $z) {
    $z->num = 10;
    $z->setRawShape($z->getInitial());
    $z->printAll();
}

demo(new Classes\Z1);



class A {
  /** @var shape(x:int, y:A) */
  public $sh;
}

$a = new A;
$a->sh = shape(['x' => 1, 'y' => null]);
