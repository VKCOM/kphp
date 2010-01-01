@unsupported
<?php

	$arr = array ("x" => "y", "a" => "b", 1 => 7);
	
	print "1\n\n\n";
	foreach ($arr as $key[0] => $val)
	{
		var_dump ($arr);
		var_dump ($key);
		var_dump ($val);
	}

	print "2\n\n\n";
	foreach ($arr as $key[0] => $val)
	{
		var_dump ($arr);
		var_dump ($key);
		var_dump ($val);
	}


	print "3\n\n\n";
	$val = array ();
	foreach ($arr as $key[0] => &$val[0])
	{
		var_dump ($arr);
		var_dump ($key);
		var_dump ($val);
	}

	print "4\n\n\n";
	$val = array ();
	foreach ($arr as $key[0] => &$val[1]) // using val1 should use the same array
	{
		var_dump ($arr);
		var_dump ($key);
		var_dump ($val);
	}


	print "5\n\n\n";
	$val = array ();
	foreach ($arr as $key[0] => &$key[1]) // what happen if you use the same value for the key and array
	{
		var_dump ($arr);
		var_dump ($key);
		var_dump ($val);
	}



?>
