@ok
<?php
	// Test of isset on 1D arrays
	$x = array();

	if(isset($x[1]))
		echo "x[1] is set (1)\n";
	
	$x[1] = 1;
	
	if(isset($x[1]))
		echo "x[1] is set (2)\n";
?>
