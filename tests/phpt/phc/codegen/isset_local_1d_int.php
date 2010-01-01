@ok
<?php
	// isset on local temps in a function (which would not be stored in 
	// a hashtable in phc compiled code)
	function f()
	{
		if(isset($x[1]))
			echo "x[1] is set (1)\n";
	
		$x[1] = 1;
	
		if(isset($x[1]))
			echo "x[1] is set (2)\n";
	}

	f();
?>
