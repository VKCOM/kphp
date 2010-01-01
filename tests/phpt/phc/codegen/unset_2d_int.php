@ok
<?php
	// Unset on 2D arrays
	$x[1][2] = 1;
	unset($x[1][2]);

	var_dump ($x);
?>
