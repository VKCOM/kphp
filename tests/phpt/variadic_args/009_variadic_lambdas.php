@ok k2_skip
<?php

$f = function($x, ...$args) {
    return (int)($x + $args[0] + array_sum($args));
};

var_dump($f(10, 20));

