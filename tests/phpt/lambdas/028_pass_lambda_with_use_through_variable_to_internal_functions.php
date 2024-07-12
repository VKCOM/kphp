@ok k2_skip
<?php

/**
 * @param int[] $scores
 * @param int $mean
 * @return float
 */
function calc_std($scores, $mean) {
    $f = function($x) use ($mean) { return ($x - $mean) * ($x - $mean); };
    return sqrt(array_sum(array_map($f, $scores)) / count($scores));
}

/**
 * @param int $mean
 */
function pow_of_diff($mean): callable {
    return function($x) use ($mean) { return ($x - $mean) * ($x - $mean); };
}

/**
 * @param int[] $scores
 * @param int $mean
 * @return float
 */
function calc_std_from_returned_lambda($scores, $mean) {
    return sqrt(array_sum(array_map(pow_of_diff($mean), $scores)) / count($scores));
}

$scores = [1, 2, 5, 8];
$mean = 4;

var_dump(calc_std($scores, $mean));
var_dump(calc_std_from_returned_lambda($scores, $mean));
