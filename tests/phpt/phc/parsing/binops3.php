@ok k2_skip
<?php

# all 4 bit permutations
foreach (range (0, 15) as $a)
{
	foreach (range (0, 15) as $b)
	{
		foreach (range (0, 15) as $c)
		{
			foreach (range (0, 15) as $d)
			{
				echo ($a & $b | $c | $d). "\n";
				echo ($a & $b | $c & $d). "\n";
				echo ($a & $b | $c ^ $d). "\n";

				echo ($a & $b & $c | $d). "\n";
				echo ($a & $b & $c & $d). "\n";
				echo ($a & $b & $c ^ $d). "\n";
				
				echo ($a & $b ^ $c | $d). "\n";
				echo ($a & $b ^ $c & $d). "\n";
				echo ($a & $b ^ $c ^ $d). "\n";
			}
		}
	}
}

?>
