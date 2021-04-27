@kphp_should_fail
/pass mixed to argument \$s of takeS/
/\$value_old is mixed/
/ternary operator is mixed/
/1 is int/
<?php

function takeS(string $s) { 1; }
function getM(any $v) : mixed { return null; }

function demo() {
    $value_old = 1 ? 1 : '1';
    if(1)
        $value_old = strlen($value_old) ? getM($value_old) . 'asdf' : $value_old;
    takeS($value_old);
}

demo();
