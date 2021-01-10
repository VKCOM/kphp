@kphp_should_fail
/pass float to argument \$x of f/
/\$arr\[\.\]\[\.\] is float/
/\$arr\[\.\] is float\[\]/
/\$arr is float\[\]\[\]/
/3.4 is float/
<?php

function f(int $x) {
}

$vv = 2;
$arr = [[1], [$vv], [3 + 5]];
$arr[0][] = 3.4;

f($arr[0][3]);
