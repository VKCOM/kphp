@kphp_should_fail
/shape\(bar:int, foo:int\) in array_diff\(\) is not supported/
<?php

function test_array_diff() {
    $arr = [shape(["foo" => 1, "bar" => 2])];
    $x = array_diff($arr, $arr);
}

test_array_diff();
