@ok
<?php

require_once 'kphp_tester_include.php';

$res = Classes\DerivedClassWithLambda::use_lambda();
var_dump($res);
