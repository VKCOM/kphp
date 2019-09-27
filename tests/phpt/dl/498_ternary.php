@ok wa
<?php
$arg = 'T';
echo ( $arg == 'B' ) ? 'bus' :
     ( $arg == 'A' ) ? 'airplane' :
     ( $arg == 'T' ) ? 'train' :
     ( $arg == 'C' ) ? 'car' :
     ( $arg == 'H' ) ? 'horse' :
                       'feet';

echo "\n";
var_dump (1 ? 1 : 0 ? 2 : 3);
var_dump (1 ? 1 : $z=1 ? 2 : 3);

$t = 0;
$x = 0 ? $z = $t : $z = $t + 1 ? $y = 5 : $y = 6;
var_dump ($x);
var_dump ($z);
var_dump ($y);

$x = 1 ? $z = 1 ? 0 : 1 : 5;
var_dump ($x);
var_dump ($z);

$x = "1";
$y = "1";
echo $x .= $y;
