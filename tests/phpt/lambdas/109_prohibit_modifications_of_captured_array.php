@kphp_should_fail
<?php

$i = [1];
$f = function($x) use ($i) { $i[0] += 1; return $x + $i[0]; };
array_map($f, [1, 2, 3]);
