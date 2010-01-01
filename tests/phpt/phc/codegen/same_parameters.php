@unsupported
<?php


	function x ($p, $p, $p)
	{
		var_dump ($p);
	}

	x (1,2,3);


	function y (&$p, $p)
	{
		var_dump ($p);
	}

	$y = 5;
	y ($y, 6);

	var_dump ($y);
?>
