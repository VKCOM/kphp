@ok
<?php

foreach (array (true, false) as $a)
{
	foreach (array (true, false) as $b)
	{
		foreach (array (true, false) as $c)
		{
			foreach (array (true, false) as $d)
			{
				echo (($a || $b || $c || $d). "\n");
				echo (($a || $b || $c && $d). "\n");
				echo (($a || $b && $c || $d). "\n");
				echo (($a || $b && $c && $d). "\n");
				echo (($a && $b || $c || $d). "\n");
				echo (($a && $b || $c && $d). "\n");
				echo (($a && $b && $c && $d). "\n");
			}
		}
	}
}

?>
