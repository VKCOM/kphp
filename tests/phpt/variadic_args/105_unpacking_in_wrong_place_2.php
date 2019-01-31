@kphp_should_fail
<?php

function f($x, ...$args) {
    var_dump($args);
}

$a = [1, 2, 3];
f(10, ...$a + 3);
