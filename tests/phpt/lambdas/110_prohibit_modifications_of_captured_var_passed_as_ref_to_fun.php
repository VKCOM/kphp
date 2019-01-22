@kphp_should_fail
<?php

function inc(&$x) {
    $x += 1;
}

$i = 10;
$f = function($x) use ($i) { inc($i); return $x + $i; };
array_map($f, [1, 2, 3]);

