<?php

define('ab', 10);

$f = function() {
    static $a = [1, 2, 3, 4];
    $a[0] += 1;
    var_dump($a);
    var_dump(ab);
};

function XXX() {
    static $a = [1, 2, 3, 4];
    var_dump($a);
    var_dump(ab);
}

$f();
$f();
XXX();
