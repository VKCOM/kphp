@kphp_should_fail
/tuple\(int, int\) in array_diff\(\) is not supported/
<?php

function test_array_diff() {
    $arr = [tuple(1, 2)];
    $x = array_diff([[1, 2]], $arr);
}

test_array_diff();
