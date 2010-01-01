@ok
<?php

	//with no cases
	$x = 0;
	echo "before\n";
	switch ($x)
	{
	}
	echo "after\n";

	// with only a default
	echo "before\n";
	switch ($x)
	{
		default:
			echo "default\n";
			break;
	}
	echo "after\n";

?>
