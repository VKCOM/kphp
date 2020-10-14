@ok
<?php
/**
 * @return int
 */
	function f()
	{
		echo "this is f\n";
		return 1;
	}

/**
 * @return int
 */
	function g()
	{
		echo "this is g\n";
		return 2;
	}

	var_dump(true ? f() : g());
	var_dump(false ? f() : g());
?>
