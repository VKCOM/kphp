@ok
<?php

	// This tests the theory that $x[] assigns to the largest integer element + 1.

	$x1["asd"] = 5;
	$x1[] = 6;
	var_dump ($x1);

	$x2[10] = 5;
	$x2[] = 6;
	var_dump ($x2);

	$x3[-10] = 5;
	$x3[] = 6;
	var_dump ($x3);

	$x4[1] = 1;
	$x4[2] = 2;
	$x4[3] = 3;
	$x4[] = 6;
	var_dump ($x4);


?>
