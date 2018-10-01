@ok
<?php

require_once("Classes/autoload.php");

function fun()
{
    return function ($a) {
        return $a->a + 10;
    };
}

$a = new Classes\IntHolder();
$res = fun()->__invoke($a);
var_dump($res);

$b = new Classes\AnotherIntHolder();
$res = fun()->__invoke($b);
var_dump($res);
