@ok
<?php

	// if you break with an expression that isnt an int, it should probably
	// evaluate to 1. SO I'm hoping that $x == 1 will report true in those
	// cases.
	if ("a string" == 0)
	{
		echo "it does\n";
	}
	else
	{
		echo "nope. Now what?\n";
	}

?>
