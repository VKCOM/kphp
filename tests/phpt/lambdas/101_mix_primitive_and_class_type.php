@kphp_should_fail
<?php

require_once("Classes/autoload.php");

$fun = function ($a) {
    return $a->a + 100;
};

$fun(new Classes\IntHolder());
$fun(10);
