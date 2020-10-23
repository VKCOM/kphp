@ok
<?php

	$x = 7;
	a ();
	b2 ();
	c2 ();

	function a ()
	{
		$x = 6;
		$y = "x";
		echo "$x\n";
	}

	function b1 ()
	{
		return "x";
	}

	function b2 ()
	{
		$x = 6;
		echo "$x\n";
	}


	function c1 ()
	{
		return "C";
	}

	function c2 ()
	{
		// this doesnt seem, to be allowed
//		var_dump (new {c1()}); // new C
	}


	// deeply nested variables
	$a = "a1";
	$a1 = "a2";
	$a2 = "a3";
	$a3 = "a4";
	$a4 = "a5";
	$b = 7;
	var_dump ($a);
	var_dump ($a1);
	var_dump ($a2);
	var_dump ($a3);
	var_dump ($a4);

?>
