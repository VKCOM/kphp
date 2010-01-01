@ok
<?php
// Global creates a reference, and must thus separate copy-on-write arguments

	$a = 5;
	$b = $a;

	function f()
	{
		global $b;
		$b = $b + 1;
	}

	f();
	
	print_r($a);
	print_r($b);
?>
