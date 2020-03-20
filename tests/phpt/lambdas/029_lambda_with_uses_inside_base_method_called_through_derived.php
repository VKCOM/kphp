@ok
<?php

require_once("polyfills.php");

$res = Classes\DerivedClassWithLambda::use_lambda();
var_dump($res);
