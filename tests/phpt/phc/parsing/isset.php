@unsupported
<?php

	$x = 10;
	$y = 20;

	if(isset($x))
	{
		echo "x is set\n";
	}
		  
	if(isset($x, $y))
	{
		echo "x, y set.\n";
	}
	
	if(isset($z))
	{
		echo "z is set";
	}
	
?>
