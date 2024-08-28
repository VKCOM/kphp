@kphp_should_fail k2_skip
/Too many arguments in call to f\(\), expected 2, have 4/
/Too many arguments in call to f\(\), expected 2, have 5/
/Too few arguments in call to g\(\), expected 2, have 1/
<?php

function f(int $s, int $i) {
    echo $s, $i;
}

f(...[1, 2, 3, 4]);
f(...[1, 2, ...[3, 4]], ...[5]);


function g($a, $b, $c = 0.0) {
}

g(...[1, ...[]]);
