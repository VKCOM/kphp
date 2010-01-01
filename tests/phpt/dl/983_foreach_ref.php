@wa
<?php
	$a = array ("a" => "A", "b" => "B", 0 => 7, "a" => "Z");
	foreach ($a as $key => &$val)
	{
		print_r ($key);
		print_r ($val);
		unset ($val);
		print_r ($a);
	}
