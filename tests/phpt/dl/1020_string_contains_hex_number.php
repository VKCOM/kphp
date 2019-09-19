@ok
<?php

// dl/457_eq2.php
// Тут дифф из-за того, что в новых версиях PHP перестали парсить строки вида
// 0xa как числа, а теперь их интерпритируют как строки, собственно пример, раньше true, теперь false
$hex_string = "0xa";
var_dump($hex_string == "1e1");
var_dump("0020" == "2e1");
var_dump("020" == "00020");
var_dump((int)$hex_string);
var_dump(intval($hex_string));
var_dump(floatval($hex_string));
var_dump(is_numeric($hex_string));
var_dump($hex_string + 5);
var_dump("0x" + 5);
var_dump(floatval("5.15asdf"));
