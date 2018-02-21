@ok
<?php
require_once 'Classes/autoload.php';

use Classes\A;
use Classes\B;

function doesNotModify(B $b) {
  $b = new B();
  $b->setB1(1);
}

(new A())->printA();
echo (new B(9))->b1, "\n";
echo B::createB(10)->getThis()->b1, "\n";

/** @var A[] $aArr */
$aArr = [new A(), (new A())->setA(10), new A()];
/** @var B[] $bArr */
$bArr = [new B(), B::createB(), $aArr[0]->createB(99)];

$aArr[0]->setA(99)->printA();
echo $aArr[2]->createB(8)->getThis()->b1, "\n";

$a0 = $aArr[0];
$a1 = (new A())->getThis();
$a1->printA();
$a2 = (new A())->setA(10);
$a2->printA();
$a3 = $aArr[2]->setA(19);
$a3->printA();
$aArr[2]->printA();

$b1 = new B();
$b2 = $b1;
echo $b1->b1, $b1->b3, "\n";
doesNotModify($b1);
echo $b1->b1, $b1->b3, "\n";
doesNotModify($bArr[1]);
$b1->setB1(1)->setB3(4);
echo $b1->b1, $b1->b3, "\n";
echo $b2->b1, $b2->b3, "\n";

$b1 = $bArr[0];
$b1->setB1(1)->setB3(4);
echo $b1->b1, $b1->b3, "\n";
echo $b2->b1, $b2->b3, "\n";
echo $bArr[0]->b1, $bArr[0]->b3, "\n";

$b3 = B::createB(88);
echo $b1->b1, ' ', $b3->setB1(89)->b1, "\n";
echo $aArr[0]->createB(88)->getThis()->setB2(9)->b1, "\n";
