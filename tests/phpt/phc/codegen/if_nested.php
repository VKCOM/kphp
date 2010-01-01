@ok
<?php
	$x = 0;
	if ($x == 1) // asda
	{
		$y = 2;
		$z = 3;
		if ($y == 4)
		{
			$z = 5;
		}
		else
		{
			$z = 6;
		}
	}
	else
	{
		$y = 7;
		if ($y == 8)
		{
			$z = 9;
		}
		else
		{
			$z = 10;
		}
	}

	var_dump ($x);
	var_dump ($y);
	var_dump ($z);
?>
