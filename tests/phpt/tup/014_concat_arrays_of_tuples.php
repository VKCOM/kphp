@ok
<?php

require_once 'kphp_tester_include.php';
$a = [tuple(10, true)];
$a = $a + $a;
var_dump(count($a));
