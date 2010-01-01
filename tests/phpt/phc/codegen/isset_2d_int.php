@ok
<?php
	// Test of isset on 2D arrays
	$x = array();

	if(isset($x[1]))
		echo "x[1] is set (1)\n";
	if(isset($x[1][2]))
		echo "x[1][2] is set (1)\n";
	if(isset($x[1][3]))
		echo "x[1][3] is set (1)\n";
	
	$x[1][2] = 1;
	
	if(isset($x[1]))
		echo "x[1] is set (1)\n";
	if(isset($x[1][2]))
		echo "x[1][2] is set (1)\n";
	if(isset($x[1][3]))
		echo "x[1][3] is set (1)\n";
?>
