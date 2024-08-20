@ok k2_skip
<?php
require_once 'kphp_tester_include.php';


$e = new Classes\E;
echo $e->nextE ? 'inited' : 'not inited', "\n";
$e->makeNextE(1);
echo $e->nextE ? 'inited' : 'not inited', "\n";
try {
  $e->makeNextE(1);
  echo $e->nextE ? 'inited' : 'not inited', "\n";
} catch (Exception $ex) {
  echo $ex->getMessage(), "\n";
}

$ee = $e->nextE->makeNextE(2)->makeNextE(3)->makeNextE(4);
$ee->a++;
echo $e->nextE->nextE->nextE->nextE->a, "\n";
echo $ee->nextE ? 'inited' : 'not inited', "\n";

$e2 = new Classes\E;
$a = $e2->makeNextE(5)->a;
$a++;
echo $a, ' ', $e2->nextE->a, "\n";
