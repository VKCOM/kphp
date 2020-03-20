@kphp_should_fail
<?php

require_once("polyfills.php");

$x = new \Classes\IntHolder(10);

$fun = function (Classes\IntHolder $a) {
    return $a->a + 100;
};

$fun(new Classes\AnotherIntHolder);
