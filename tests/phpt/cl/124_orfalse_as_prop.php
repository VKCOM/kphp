@ok
<?php
require_once 'kphp_tester_include.php';


$a = new Classes\A;
$a->a = false;
if ($a->a)
  $a->printA();
$a->a = 3;
if ($a->a)
  $a->printA();
