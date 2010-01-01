@ok
<?php
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
