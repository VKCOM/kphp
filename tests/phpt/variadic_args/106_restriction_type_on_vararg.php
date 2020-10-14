@kphp_should_fail
<?php

/**
 * @param int $acc
 * @param int[] $args
 */
function sum_of_ints($acc, ...$args) {
    return $acc + (int) array_sum($args);
}

sum_of_ints(10, 1, 2, 3.2);
