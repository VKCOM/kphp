@kphp_should_fail
<?php

function g($x) { return function($a, $b) { return $a + $b + 10; }; }

$a = array_map(g(10), [1, 2, 3]);

var_dump($a);

