@ok
<?php

function getBits() {
    return 1 ? 10 : false;
}

function demo() {
    $mask = 0;
    $mask |= $bits = getBits();

    if($bits === false) {
        echo "false\n";
    } else {
        echo $bits, "\n";
    }
}

demo();

$x = 1.0;
echo "$x\n";

$y = $x += 2;
echo "$x\n";
echo "$y\n";

$z = $y *= $x -= 2;
echo "$x\n";
echo "$y\n";
echo "$z\n";

$x /= $y += $z *= 4;
echo "$x\n";
echo "$y\n";
echo "$z\n";

$y = $x += $z -= $y;
echo "$x\n";
echo "$y\n";
echo "$z\n";

$z |= $x &= $y *= $z += 1;
echo "$x\n";
echo "$y\n";
echo "$z\n";
