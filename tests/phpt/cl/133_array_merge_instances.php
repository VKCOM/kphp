@ok k2_skip
<?php
require_once 'kphp_tester_include.php';
use Classes\B;

$arr1 = [3=>new B(1), 5=>new B(2)];
$arr2 = [10=>new B(3), 4=>new B(4), 33=>new B(5)];

/** @var B[] */
$arr = array_values(array_merge($arr1, $arr2));
foreach($arr as $b) {
    echo $b->b1, "\n";
}

$d1 = new Classes\D;
$d2 = new Classes\D;
$d1->cArr[0]->c1 = 10;
$d2->cArr[0]->c1 = 20;
/** @var Classes\C[] */
$carr = array_merge($d1->getCArr(), $d2->getCArr());
foreach($carr as $c) {
    echo $c->c1, "\n";
}
