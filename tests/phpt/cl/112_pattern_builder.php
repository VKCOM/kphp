@ok
<?php
require_once 'kphp_tester_include.php';

use Classes\A;

$a1 = (new A())->getThis()->setA(10)->getThis();
$a1->printA();
$a2 = (new A())->setA(1);
$a3 = $a2->setA(4);
$a4 = $a2->getThis()->setA(8)->getThis();
$a5 = $a4->setA(90)->getThis();
$a1->printA();
$a2->printA();
$a3->printA();
$a4->printA();
$a5->printA();
