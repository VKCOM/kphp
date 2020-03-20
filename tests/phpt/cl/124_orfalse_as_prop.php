@ok
<?php
require_once 'polyfills.php';


$a = new Classes\A;
$a->a = false;
if ($a->a)
  $a->printA();
$a->a = 3;
if ($a->a)
  $a->printA();
