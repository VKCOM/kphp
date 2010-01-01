@ok
<?php

	// If they're just copied in, they actually copies, right?

	$y = 123;

	$x[0] = $y;
	$x["string"] = $y;

	var_dump ($x);
	$x["string"] = 5; // set $x[0] too
	var_dump ($x);

	// Yes (is_ref is not set, so they are correctly separated).
?>
