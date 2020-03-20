@ok
<?php

require_once('polyfills.php');
use Classes\B;

$b = new Classes\A;
$b = new Classes\B;
var_dump($b instanceof Classes\B);
var_dump($b instanceof B);
var_dump($b instanceof \Classes\B);
var_dump($b instanceof Classes\A);
