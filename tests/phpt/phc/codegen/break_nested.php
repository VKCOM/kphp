@ok
<?php

	for ($i = 0; $i < 10; $i++)
	{
		var_dump($i);
		for ($j = 0; $j < 10; $j++)
		{
			var_dump($j);
			break;
			var_dump("XXX");
		}
		if ($i == 5)
			break;
	}
?>
