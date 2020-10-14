@ok
<?php

/**
 * @param mixed $x
 */
	function f(&$x)
	{
		$x = 7;
	}

	$y = "test";
	f ($y);

?>
