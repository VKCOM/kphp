@kphp_should_fail
<?php

require_once 'kphp_tester_include.php';

$fun = function ($a) {
    return $a->a + 100;
};

$fun(new Classes\IntHolder());
$fun(10);
