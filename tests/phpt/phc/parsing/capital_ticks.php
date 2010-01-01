@unsupported
<?php

	function f ()
	{
		static $i = 0;
		echo "tick: $i\n";
		$i++;
	}

	register_tick_function ("f");

	declare (ticks = 1)
	{
		for ($i = 0; $i < 100; $i++)
		{
			echo "i1: $i\n";
		}
	}

	// does it work the same when capitalized
	declare (TICKS = 1)
	{
		for ($i = 0; $i < 100; $i++)
		{
			echo "i2: $i\n";
		}
	}
?>
