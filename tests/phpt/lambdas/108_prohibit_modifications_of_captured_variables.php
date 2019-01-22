@kphp_should_fail
<?php

$i = 10;
$f = function($x) use ($i) { $x = 10; $i += 1; return $x + $i; };
array_map($f, [1, 2, 3]);
