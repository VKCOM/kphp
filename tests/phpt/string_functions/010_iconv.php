@ok binary
<?php

$original = "Привет мир";
$step1 = iconv('UTF-8', 'windows-1251', $original);
var_dump($step1);
$step2 = iconv('UTF-8', 'windows-1251', $step1);
var_dump($step2);

$empty = "";
var_dump(iconv('UTF-8', 'windows-1251', $empty));

// from examples: https://www.php.net/manual/en/function.iconv.php
$text = "This is the Euro symbol '€'.";
var_dump($text);
// TODO a strange bug when comparing via kphp_tester.py
//var_dump(iconv("UTF-8", "ISO-8859-1//TRANSLIT", $text));
// TODO works on MacOS but does not work on Linux
//var_dump(iconv("UTF-8", "ISO-8859-1//IGNORE", $text));
var_dump(iconv("UTF-8", "ISO-8859-1", $text));
