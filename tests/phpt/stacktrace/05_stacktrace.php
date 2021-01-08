@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/return array< string > from getArr/
/array is array< string >/
/concatenation is string/
/assign mixed to \$i/
/ternary operator is mixed/
/getArr returns array< int >/
/getArr declared that returns array< int >/
<?php

/** @var string[] $i */
$i = [];


function getArr(): int[] {
    return ['9' . 2];
}

$i = 1 ? getArr() : 's';
