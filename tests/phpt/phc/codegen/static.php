@ok
<?php
/*
 * Test of the "static" keyword
 */

	function f()
	{
		static $x = 0;
		echo $x;
		$x++;
	}

	f();
	f();
?>
