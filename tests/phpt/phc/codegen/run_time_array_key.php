@ok
<?php

/**
 * @kphp-infer
 * @return int
 */
	function f()
	{
		return 17;
	}

	$x = array ("asd" => f(), f() => 5, "x"."x" => 7 );
	var_dump ($x);
?>
