@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/return string\[\] from getArr/
/array is string\[\]/
/concatenation is string/
/assign mixed to \$i/
/ternary operator is mixed/
/getArr returns int\[\]/
/getArr declared that returns int\[\]/
<?php

/** @var string[] $i */
$i = [];


function getArr(): int[] {
    return ['9' . 2];
}

$i = 1 ? getArr() : 's';
