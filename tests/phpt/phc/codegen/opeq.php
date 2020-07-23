@ok
<?php
// Demonstrates that $x += $y cannot be represented as $x = $x + $y
/**
 * @kphp-infer
 * @return int
 */
	function f()
	{
		echo "f is called\n";
		return 1;
	}

	$x[1] = 10;
	$x[f()] += 1;

	var_dump($x);
?>
