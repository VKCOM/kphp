@ok php8
<?php

// See https://wiki.php.net/rfc/trailing_comma_in_closure_use_list

$a = 'test1';
$b = 'test2';
$fn = function () use (
    $a,
    $b,
) {
   echo $a, $b;
};

$fn();
