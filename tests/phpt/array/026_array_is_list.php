@skip php8
<?php

var_dump(array_is_list([]));
var_dump(array_is_list([1, 2]));
var_dump(array_is_list([1, 'x', 2]));
var_dump(array_is_list(['a']));
var_dump(array_is_list([[1], [2]]));
var_dump(array_is_list([['a']]));
var_dump(array_is_list([0 => 1, 1 => 2]));

var_dump(array_is_list(['a' => 10]));
var_dump(array_is_list([0 => 1, 1 => 2, 'a' => 10]));
var_dump(array_is_list([1 => 2]));
var_dump(array_is_list([1 => 'a', 0 => 'b']));

$a = [1, 2, 3];
unset($a[1]);
$a[1] = 10;
var_dump(array_is_list($a));

$a2 = [];
$a2[] = 1;
var_dump(array_is_list($a2));
$a2[] = 2;
var_dump(array_is_list($a2));
unset($a[0]);
var_dump(array_is_list($a2));
