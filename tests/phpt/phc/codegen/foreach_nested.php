@ok
<?php

	$a = array ("a" => "A", "b" => "B", 0 => 7, "a" => "Z");
	$b = array ("w" => "W", "x" => "X", 5 => 12, "d" => "D");
	foreach ($a as $x)
	{
		print_r ($a);
		print_r ($x);
		foreach ($b as $y)
		{
			print_r ($b);
			print_r ($y);
		}
	}

	// with a break
	foreach ($a as $x)
	{
		print_r ($a);
		print_r ($x);
		foreach ($b as $y)
		{
			print_r ($b);
			break;
			print_r ($y);
		}
	}
?>
