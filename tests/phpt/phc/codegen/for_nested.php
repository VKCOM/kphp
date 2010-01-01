@ok
<?php
	for($i = 0; $i < 10; $i = $i + 1)
	{
		echo "$i\n";
		for($j = 0; $j < 10; $j = $j + 1)
		{
			echo "$j\n";
			for($k = 20; $k > 0; $k = $k - 1)
			{
				echo "$k\n";
			}
		}
	}

	var_dump ($i);
	var_dump ($j);
	var_dump ($k);
?>
