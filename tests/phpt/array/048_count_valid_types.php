@ok
<?php

/** @param mixed $value */
function count_mixed($value): int {
    return count($value);
}

function count_optional(?array $value): int {
    return count($value);
}

$items = [1, 2, 3];
var_dump(count($items));
var_dump(sizeof($items));
var_dump(count_mixed($items));
var_dump(count_optional($items));
var_dump(count([]));
