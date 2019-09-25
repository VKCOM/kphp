@kphp_should_fail
<?php
require_once 'Classes/autoload.php';

// kphp fails, because array(1, new A) is array<var>, and list(,$a) = ... — $a is var, not class_instance
// so, returning mixed array is a bad pattern; you should return a typed class instead

use Classes\A;
use Classes\B;

/** @return A [] */
function getAArr() {
  return array(new A(), new A);
}

function getCountAndBArr() {
  return array(2, array(new B(), new B(11, 22)));
}

/** @var A $a */
list($count, $a) = array(1, (new A())->setA(19));
/** @var $aArr Classes\A[] */
list($count2, $aArr) = array(2, array(new A(), new A()));

$a->printA();
$aArr[1]->setA(8)->printA();
$a2 = getAArr()[0];
$a2->setA(2)->printA();

/** @var Classes\B[] $bArr */
list($count3, $bArr) = getCountAndBArr();
echo $bArr[1]->b1, "\n";
