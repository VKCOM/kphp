@ok
<?php

	// if we simply call exit, this memory leak wont be cleaned up

	function f () {
		$x = 5;
		exit (0);
	}

	f ();
?>
