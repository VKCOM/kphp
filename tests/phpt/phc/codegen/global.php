@ok
<?php
	function f()
	{
		global $x;
		$x = 10;
	}

	function g() {
	  $x = 20;
	}

	f();
	var_dump($x);
	g();
	var_dump($x);

?>
