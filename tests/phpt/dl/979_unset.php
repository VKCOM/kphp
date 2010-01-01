@unsupported
<?php
	$x = "a string";
#	unset ($x[3]);

	var_dump ($x);

	$x = array (0 => "x", 1 => "z", "z" => "a");
	unset ($x [NULL]);

	var_dump (count ($x));

	$scalar_array = array (
		"",
		"a",
		"ab",
		"aasd",
		"really quite a really really long string, longer even",
		array (),
		array (17, 45),
		array (17 => "asd", 45 => ""),
		array ("ppp", "jjj"),
		array ("ppp" => 17, "jjj" => 45),
		array ("2", "3"),
		array ("2" => 2, "3" => 3),
		array (array ("rrr")),
		56,
		-17,
		0,
		1,
		-1,
		62.75,
		0.0,
		-0.0,
		NULL,
		false,
		true);

	$short_scalar_array = array (
		"",
		"really quite a re",
		array ("2", 3, "four"),
		0,
		1,
		-1,
		62.75,
		0.0,
		-0.0,
		NULL,
		false,
		true);
  
  print_r ($scalar_array);
  print_r ($short_scalar_array);

	$scalar_array = $short_scalar_array;

	foreach ($scalar_array as $scalar)
	{
		foreach ($scalar_array as $i)
		{
			echo "Working with ";
			var_dump ($scalar);

			echo "\nUnset single level (", gettype($i), ": $i)\n";
			$x = $scalar;

			// Ignore strings
			if (is_string ($x))
			{
				echo "skip\n";
				continue;
			}

			unset ($x[$i]);
			var_dump ($x);

			foreach ($scalar_array as $j)
			{
				echo "\nUnset two levels (", gettype ($i), ": $i, ", gettype ($j), ": $j)\n";
				$x = $scalar;

				// Ignore strings
				if (is_string ($x[$i]))
				{
					echo "skip\n";
					continue;
				}

				unset ($x[$i][$j]);
				var_dump ($x);

				foreach ($scalar_array as $k)
				{
					echo "\nUnset 3 levels (", gettype ($i), ": $i, ", gettype ($j), ": $j, ", gettype ($k), ": $k))\n";
					$x = $scalar;

					// Ignore strings
					if (is_string ($x[$i][$j]))
					{
						echo "skip\n";
						continue;
					}

					unset ($x[$i][$j][$k]);
					var_dump ($x);
				}
			}
		}
	}
	
?>
