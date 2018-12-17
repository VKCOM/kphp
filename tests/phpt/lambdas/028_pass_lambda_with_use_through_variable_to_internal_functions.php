@ok
<?php

function calc_std($scores, $mean) {
    $f = function($x) use ($mean) { return ($x - $mean) * ($x - $mean); };
    return sqrt(array_sum(array_map($f, $scores)) / count($scores));
}

function pow_of_diff($mean) {
    return function($x) use ($mean) { return ($x - $mean) * ($x - $mean); };
}

function calc_std_from_returned_lambda($scores, $mean) {
    return sqrt(array_sum(array_map(pow_of_diff($mean), $scores)) / count($scores));
}

$scores = [1, 2, 5, 8];
$mean = 4;

var_dump(calc_std($scores, $mean));
var_dump(calc_std_from_returned_lambda($scores, $mean));
