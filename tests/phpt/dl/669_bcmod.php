@ok k2_skip
<?php

echo bcmod(16, 7), "\n";
echo bcmod(16, 8), "\n";
echo bcmod(16, 9), "\n";

echo bcmod('', '3', 2), "\n";
echo bcmod('0', '3', 2), "\n";

echo bcmod('2', '3'), "\n";
echo bcmod('2', '3', 0), "\n";
echo bcmod('2', '3', 1), "\n";
echo bcmod('2', '3', 2), "\n";
echo bcmod('2', '3', 3), "\n";
echo bcmod('2', '3', 4), "\n";
echo bcmod('2', '3', 200), "\n";

echo bcmod('-2', '3'), "\n";
echo bcmod('-2', '3', 0), "\n";
echo bcmod('-2', '3', 1), "\n";
echo bcmod('-2', '3', 2), "\n";
echo bcmod('-2', '3', 3), "\n";
echo bcmod('-2', '3', 4), "\n";
echo bcmod('-2', '3', 200), "\n";
