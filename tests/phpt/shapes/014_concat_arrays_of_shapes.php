@ok
<?php

require_once 'kphp_tester_include.php';

$a = [shape(['int' => 10, 'b' => true])];
$b = [shape(['int' => 20, 'b' => true])];
$c = array_merge($a, $b);
var_dump($c[0]['int']);
var_dump($c[1]['int']);
