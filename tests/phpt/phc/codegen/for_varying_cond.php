@ok
<?php

	// f ($x) should be recalculated every iteration
	for ($x = 0; f ($x); $x++)
	{
		var_dump ($x);
	}

	function f ($y)
	{
		return ($y != 7);
	}

?>
