@ok php8
<?php

require_once 'kphp_tester_include.php';

$a = 'test1';
$b = 'test2';
$fn = function () use (
    $a,
    $b,
) {
   echo $a, $b;
};

$fn();