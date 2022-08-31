@ok K.O.T.
<?php

echo bcpow (16, 7)."\n";
echo bcpow (16, 8)."\n";
echo bcpow (16, 9)."\n";

echo bcpow('', '3', 2), "\n";
echo bcpow('0', '3', 2), "\n";
echo bcpow('2', '', 2), "\n";
echo bcpow('2', '0', 2), "\n";

echo bcpow('2', '3'), "\n";
echo bcpow('2', '3', 0), "\n";
echo bcpow('2', '3', 1), "\n";
echo bcpow('2', '3', 2), "\n";
echo bcpow('2', '3', 3), "\n";
echo bcpow('2', '3', 4), "\n";
echo bcpow('2', '3', 200), "\n";

echo bcpow('-2', '3'), "\n";
echo bcpow('-2', '3', 0), "\n";
echo bcpow('-2', '3', 1), "\n";
echo bcpow('-2', '3', 2), "\n";
echo bcpow('-2', '3', 3), "\n";
echo bcpow('-2', '3', 4), "\n";
echo bcpow('-2', '3', 200), "\n";
