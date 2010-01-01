@ok
<?php
	// Test of isset on 1D arrays using string indices
	$x = array();

	if(isset($x["a"]))
		echo "x[a] is set (1)\n";
	
	$x["a"] = 1;
	
	if(isset($x["a"]))
		echo "x[a] is set (2)\n";
?>
