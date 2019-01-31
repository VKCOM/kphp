@kphp_should_fail
<?php

function mult($x, $y) {
    var_dump($x, $y);
}

$arr = [5, 4];
mult(...$arr);
