@ok
<?php

require_once 'polyfills.php';
$a = [tuple(10, true)];
$a = $a + $a;
var_dump(count($a));
