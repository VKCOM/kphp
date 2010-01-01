@ok no_php
<?php
	function f()
	{
		echo "called f\n";
		return 1;
	}
	
	function g()
	{
		echo "called g\n";
		return 2;
	}

	$a[f()] = g();
	var_dump($a);
?>
