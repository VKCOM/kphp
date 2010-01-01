@wa
<?php
  require ("982_indexing_non_arrays_arr.php");

	function php_write ($php_init, $insert)
	{
		echo "--------------------\n";
		echo "checking early php_write\n";
		$x = $php_init;
		$x[3] = $insert; // php_write in string
		var_dump ($x[2]);  // read val before
		var_dump ($x[3]); // read same val
		var_dump ($x[4]);  // read val after
		var_dump ($x[17]); // read out of string
		var_dump ($x);

		echo "--------------------\n";
		echo "checking late php_write\n";
		$x = $php_init;
		$x[17] = $insert;
		var_dump ($x[17]); // read same val
		var_dump ($x);

		echo "--------------------\n";
		echo "checking very early php_write\n";
		$x = $php_init;
		$x[-3] = $insert;
		var_dump ($x[-3]); // read same val
		var_dump ($x[-1]); // read early
		var_dump ($x);


		if (is_string ($php_init))
			return; // not supported, move to separate test case

		echo "--------------------\n";
		echo "checking early (ref) php_write\n";
		$x = $php_init;
		$x[3] = $insert; // php_write in string
		var_dump ($x[2]);  // read val before
		var_dump ($x[3]); // read same val
		var_dump ($x[4]);  // read val after
		var_dump ($x[170]); // read out of string
		var_dump ($x);

		echo "--------------------\n";
		echo "checking late (ref) php_write\n";
		$x = $php_init;
		$x[170] = $insert;
		var_dump ($x[170]); // read same val
		var_dump ($x);
	}
	
	function php_push ($php_init, $insert)
	{
		if (is_string ($php_init))
			return; // not supported. Move to separate test case

		// early and late dont make sense here
		echo "--------------------\n";
		echo "checking php_push\n";
		$x = $php_init;
		$x[] = $insert; // php_write in string
		var_dump ($x[2]);  // read val before
		var_dump ($x[3]); // read same val
		var_dump ($x[4]);  // read val after
		var_dump ($x[17]); // read out of string
		var_dump ($x);

		echo "--------------------\n";
		echo "checking (ref) php_push\n";
		$x = $php_init;
		$x[] = $insert; // php_write in string
		var_dump ($x[2]);  // read val before
		var_dump ($x[3]); // read same val
		var_dump ($x[4]);  // read val after
		var_dump ($x[170]); // read out of string
		var_dump ($x);
	}

	foreach ($scalar_array as $php_init)
	{
		foreach ($scalar_array as $insert)
		{
			echo "--------------------\n";
			echo "php_init: ";
			var_dump ($php_init);
			echo "Insert: ";
			var_dump ($insert);

			php_write ($php_init, $insert);
			php_push ($php_init, $insert);
		}
	}

	foreach ($short_scalar_array as $scalar)
	{
		var_dump ($scalar);
		foreach ($scalar as $y => $z)
		{
			echo "$y => $z\n";
		}
	}

?>
