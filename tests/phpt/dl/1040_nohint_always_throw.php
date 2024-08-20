@kphp_should_fail k2_skip
/\$i = getInt\(2\);/
/Using result of void function getInt/
/echo \$i, "\\n ";/
/Using result of void expression/
<?php

function getVoid() {
    throw new Exception();
}

function getInt(int $i) {
    switch ($i) {
    case 1: throw new Exception();
    case 2: exit();
    default: die();
    }
    return 1;
}

getVoid();
$i = getInt(2);
echo $i, "\n";
