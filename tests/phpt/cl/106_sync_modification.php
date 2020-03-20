@ok
<?php
require_once 'polyfills.php';

use Classes\A;

function setA(A $a, $val) {
  $a->setA($val)->printA();
}

/** @type A[] $aArr */
$aArr = [new A(), new A(), new A()];
$a0 = $aArr[0];
$a1 = $aArr[1];
$a2 = $aArr[2];
$a00 = $a0;
/** @var $a000 A[] */
$a000 = array($a00);

$a1->printA();
$aArr[1]->setA(90)->printA();
$a1->printA();

setA($a2, 234);
$aArr[2]->printA();

setA($aArr[2], 812);
$aArr[2]->printA();

setA($a000[0], 123);
$aArr[0]->printA();
$a00->printA();

echo $a1->a, $a2->a, $a00->a, $a0->a, $a000[0]->a, "\n";
