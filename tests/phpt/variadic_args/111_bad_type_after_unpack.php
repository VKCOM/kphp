@kphp_should_fail
/pass int to argument \$s of f/
<?php

function f(string $s, int $i) {
    echo $s, $i;
}

f(...[1, 2]);
