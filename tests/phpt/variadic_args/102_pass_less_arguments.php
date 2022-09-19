@kphp_should_fail
/It's prohibited to unpack non-fixed arrays where positional arguments expected/
<?php

function fun($x, ...$args) {
    var_dump($x);
    var_dump($args);
}

$a = [1, 2, 3];
fun(...$a);
