@ok
<?php

	for ($i = 0; $i < 10; $i++)
	{

		for ($j = 0; $j < 10; $j++)
		{
			for ($k = 0; $k < 10; $k++)
			{
				var_dump ("i$i");
				var_dump ("j$j");
				var_dump ("k$k");
				break;
				var_dump ("i$i");
				var_dump ("j$j");
				var_dump ("k$k");
			}
		}

	}
?>
