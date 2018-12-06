@ok
<?php

require_once 'polyfill/tuple-php-polyfill.php';
$a = [tuple(10, true)];
$a = $a + $a;
var_dump(count($a));
