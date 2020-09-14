@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function getMe() {
    if(1)
        return shape(['i' => 1, 's' => 'str']);
    return tuple(1, 'str', 'more');
}

function demo() {
    $t = getMe();
}

demo();
