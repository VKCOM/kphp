@ok
<?php

function g($x) { return function($a) { return $a + 10; }; }

$a = array_map(g(10), [1, 2, 3]);

var_dump($a);

