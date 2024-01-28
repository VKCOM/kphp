@ok binary
<?php

$original = "Привет мир";
$step1 = iconv('UTF-8', 'windows-1251', $original);
var_dump($step1);
$step2 = iconv('UTF-8', 'windows-1251', $step1);
var_dump($step2);

$empty = "";
var_dump(iconv('UTF-8', 'windows-1251', $empty));
