@kphp_should_fail
<?php

function test() {
    $a = [1, 2, 3];
    $x = array_column($a, 2);
}

test();
