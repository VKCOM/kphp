@ok
<?php

	// We cant lower a do_while into a while due to continue statements. We can convert whiles into do-whiles.

	$x = 10;
	while ($x)
	{
		$x--;
		if ($x < 3)
			continue;
		echo "x\n";
	}

	// this converts into the following
	$x = 10;
	do
	{
		if (!$x) break;
			$x--;

		if ($x < 3)
			continue;

		echo "x\n";
	}
	while (true);

?>
