@ok
<?php

	$a = array ("a" => "A", "b" => "B", 0 => 7, "a" => "Z");
	echo ("=============================================\n");
	foreach ($a as $key => $val)
	{
		print_r ($key);
		print_r ($val);
		print_r ($a);
	}

	// same again with referenced val
	echo ("=============================================\n");
	foreach ($a as $key => &$ref)
	{
		print_r ($key);
		print_r ($ref);
		print_r ($a);
	}
	unset ($ref);
	// add some unsets
	echo ("=============================================\n");
	foreach ($a as $key => $val)
	{
		print_r ($key);
		print_r ($val);
		unset ($val);
		print_r ($a);
	}
?>
