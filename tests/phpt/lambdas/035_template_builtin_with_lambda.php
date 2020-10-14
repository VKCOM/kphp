@ok
<?php

/**
 * @param int[] $ints
 * @return int
 */
function get_sum($ints) {
    return (int)array_sum($ints);
}

function run() {
    $arr = [10.123, '123.98'];
    $ints = array_map('intval', $arr);
    var_dump(get_sum($ints));
}

run();

