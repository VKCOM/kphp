@ok
<?php
require_once 'Classes/autoload.php';

use Classes\A;

/** @return A[] */
function getAArr() {
  return array((new A())->setA(11), (new A)->setA(22)->getThis());
}

/**
 * @param $aArr A [] this is a arr
 */
function printAForeach($aArr) {
  foreach ($aArr as $a)
    $a->printA();
}

/**
 * @param A [] $aArr this is a arr
 */
function printAForeach2($aArr) {
  foreach ($aArr as $a)
    $a->getThis()->printA();
}

$aArr = getAArr();
foreach ($aArr as $a1)
  $a1->printA();

foreach (getAArr() as $a2)
  $a2->printA();

/** @type A[] $aArr2 */
$aArr2 = array(new A(), new A(), (new A)->setA(8));
$aArr3 = $aArr2;
foreach ($aArr3 as $a3)
  $a3->printA();

printAForeach($aArr);
printAForeach2($aArr2);

for ($i = 0; $i < count($aArr); ++$i)
  $aArr[$i]->printA();
