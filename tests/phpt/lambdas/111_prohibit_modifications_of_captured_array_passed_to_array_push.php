@kphp_should_fail
<?php

$f = function($x) use ($i) { array_push($i, 2); return $x + $i[0]; };
array_map($f, [1, 2, 3]);
