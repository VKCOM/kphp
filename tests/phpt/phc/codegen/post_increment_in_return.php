@ok UB
<?php
	$i = 0;

/**
 * @return int
 */
	function f()
	{
		global $i;
		return $i++ * 2;
	}

	var_dump(f() + $i);
	var_dump(f() + $i);
	var_dump(f() + $i);
?>
