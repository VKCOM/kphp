@ok
<?php

require_once('Classes/autoload.php');
use Classes\B;

$b = new Classes\A;
$b = new Classes\B;
var_dump($b instanceof Classes\B);
var_dump($b instanceof B);
var_dump($b instanceof \Classes\B);
var_dump($b instanceof Classes\A);
