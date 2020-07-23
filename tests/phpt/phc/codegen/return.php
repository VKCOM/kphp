@ok
<?php
/**
 * @kphp-infer
 * @return int
 */
	function f()
	{
		return 5;
	}

	function g()
	{
		return;
	}

	var_dump(f());
	var_dump(g());
?>
