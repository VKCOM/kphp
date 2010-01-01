@ok
<?php
	$x = 10;
	$y = 10;
	$z = 10;
	
	// Unary -
	$x = - $x + 1;

	// Post-increment
	$x = 2 + $y++ ;

	// Pre-decrement
	$x = 3 + --$z;

	var_dump($x);
	var_dump($y);
	var_dump($z);
?>
