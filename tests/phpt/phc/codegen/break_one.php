@ok
<?php

	echo "before\n";
	for ($i = 0; $i < 3; $i++)
	{
		var_dump ("i$i");
		for ($j = 0; $j < 3; $j++)
		{
			var_dump ("j$j");
			for ($k = 0; $k < 3; $k++)
			{
				var_dump ("k$k");
				for ($m = 0; $m < 3; $m++)
				{
					var_dump ("m$m");
					break;
				}
			}
		}
	}

	echo "after\n";
?>
