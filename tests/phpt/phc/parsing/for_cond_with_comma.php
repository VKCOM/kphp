@ok
<?php
/**
 * @kphp-infer
 * @param string $id
 * @param bool $x
 * @param bool $y
 */
	function f($id, $x, $y)
	{
		for(;$x,$y;)
		{
			echo "$id\n";
			break;
		}
	}

	f("a", false, false);
	f("b", false, true);
	f("c", true, false);
	f("d", true, true);
?>
