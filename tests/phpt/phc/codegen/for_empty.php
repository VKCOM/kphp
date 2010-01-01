@ok
<?php

	// for loops dont work properly
	//$x = 0;
	for (;;)
	{
		$x ++;
		var_dump ($x);
		// we need to exit
		if ($x > 10)
			exit ();
	}
?>
