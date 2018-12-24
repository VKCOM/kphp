@ok
<?php

require_once("Classes/autoload.php");

$res = Classes\DerivedClassWithLambda::use_lambda();
var_dump($res);
