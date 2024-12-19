@ok
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

echo bcpow('abcd', '2'), "\n";
echo bcpow('abcd', '2', 2), "\n";
echo bcpow('2', 'abcd'), "\n";
echo bcpow('2', 'abcd', 2), "\n";

echo bcpow('2', '1.23'), "\n";
echo bcpow('2', '-1.23'), "\n";
echo bcpow('2', '0.952'), "\n";
echo bcpow('2', '-0.952'), "\n";
echo bcpow('2', '12.23'), "\n";
echo bcpow('2', '-12.23'), "\n";
echo bcpow('2', '934.023'), "\n";
echo bcpow('2', '-934.023'), "\n";

echo bcpow('2', '0'), "\n";
echo bcpow('2', '0', 4), "\n";
echo bcpow('2', '+0'), "\n";
echo bcpow('2', '+0', 9), "\n";
echo bcpow('2', '-0'), "\n";
echo bcpow('2', '-0', 2), "\n";
echo bcpow('2', '+0.0'), "\n";
echo bcpow('2', '-0.0'), "\n";
echo bcpow('2', '-0.0', 5), "\n";

echo bcpow('0', '2'), "\n";
echo bcpow('0', '2', 4), "\n";
echo bcpow('+0', '2'), "\n";
echo bcpow('+0', '2', 9), "\n";
echo bcpow('-0', '2'), "\n";
echo bcpow('-0', '2', 2), "\n";
echo bcpow('+0.0', '2'), "\n";
echo bcpow('-0.0', '2'), "\n";
echo bcpow('-0.0', '2', 5), "\n";

echo bcpow('', 2), "\n";
echo bcpow('', 2, 5), "\n";
echo bcpow(2, ''), "\n";
echo bcpow(2, '', 5), "\n";
echo bcpow('', ''), "\n";
echo bcpow('', '', 5), "\n";
