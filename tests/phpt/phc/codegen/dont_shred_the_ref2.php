@ok
<?php

	// If we say we can shred references if only all our assignments are by
	// reference, we get a warning here. (If running this test, and the test
	// doesnt assign to temporaroes by reference, then dont forget to add &
	// after assignments from variables).

	$x = 7;
	@var_dump ($x[0]);
	var_dump ($x);

?>
