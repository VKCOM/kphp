@ok
<?php
require_once 'polyfills.php';

use Classes\C;

/** @return Classes\B */
function modify(Classes\B $b) {
  return $b->setB1(90)->setB2(89)->setB3(88);
}

$c = new C();
$c->aInst->setA(2)->printA();
$c->addThis();

$a = $c->aInst;
$a->setA(89)->printA();
$c->aInst->printA();
$c->getAInst()->printA();
$c->createNextC();

$b = $c->getBInst();
echo $b->b1, ' ', $b->b3, "\n ";
$b2 = modify($c->getBInst());
echo $b->b1, ' ', $b->b3, "\n ";
echo $b2->b1, ' ', $b2->b3, "\n ";

echo $c->getBInst()->setB3(111)->b3, "\n ";
echo $b2->b3, "\n ";

$d = new Classes\D;
$a10 = $d->cInst->aInst;
$a10->printA();
$d->cInst->aInst->getThis()->printA();
echo $d->cInst->getAInst()->createB(39)->b1, "\n ";
echo $d->cInst->aInst->a, "\n ";
$d->cInst->aInst->a = 90;
$a10->printA();

$a20 = $d->cArr[0]->aInst;
$a20->printA();
$d->cArr[0]->aInst->getThis()->printA();
echo $d->getCArr()[0]->getAInst()->createB(39)->b1, "\n ";
echo $d->getCArr()[0]->aInst->a, "\n ";
// (Can not mix [] and -> as lvalue for now) $d->cArr[0]->aInst->a = 90;
$a10->printA();

