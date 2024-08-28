@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

$b = new Classes\PassCallbackInsideClass;
$b->use_callback();

$a = new Classes\UseAnotherClassInsideLambda();
$a->use_another_class_inside_lambda();

$x = Classes\SelfInsideLambda::run();
var_dump($x);
