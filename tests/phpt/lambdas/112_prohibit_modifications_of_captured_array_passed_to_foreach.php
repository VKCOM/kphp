@kphp_should_fail
<?php

$i = [10];
$f = function($x) use ($i) {
    foreach($i as &$it) { $it += 1; }
    return $i[0];
};
var_dump(array_map($f, [1, 2, 3]));
