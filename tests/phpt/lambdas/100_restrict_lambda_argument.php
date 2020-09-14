@kphp_should_fail
<?php

require_once 'kphp_tester_include.php';

$x = new \Classes\IntHolder(10);

$fun = function (Classes\IntHolder $a) {
    return $a->a + 100;
};

$fun(new Classes\AnotherIntHolder);
