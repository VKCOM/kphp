@kphp_should_fail
<?php

function test_access_to_captured_vars() {
    $x = 10;
    $f = function() use ($x) {};
    var_dump($f->x);
}

test_access_to_captured_vars();

