@ok
<?php

	// This adds nothing to $x. If we add a
	// temporary with a reference, we get
	// wrong code (a NULL gets added to $x)

	function foo ($y)
	{
	}
	
	$x = array (1);
	foo ($x[0]);
	var_dump($x);
?>
