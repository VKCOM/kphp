@ok
<?php

$x = 10;
$y = 20;

function foo() {
	global $x, $y;

	var_dump($x);
	var_dump($y);
}

foo();

?>
