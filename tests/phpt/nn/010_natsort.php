@ok k2_skip
<?php
/*
* proto bool natsort ( array &$array )
* Function is implemented in ext/standard/array.c
*/
$array1 = $array2 = array("img12.png", "img15.png", "img2.png", "img100.png");
sort($array1);
echo "Standard sorting\n";
print_r($array1);
natsort($array2);
echo "\nNatural order sorting\n";
print_r($array2);
?>
