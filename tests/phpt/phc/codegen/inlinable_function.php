@ok
<?php

/**
 * @return string
 */
	function a () 
	{
		$x=1;
		$y=$x+1;
		
		var_dump ($y);		

		return "a";
	}


/**
 * @return string
 */
	function b ()
	{
		return "b";
	}

	$a = a();
	$b = b();

	var_dump ($a);
	var_dump ($b);


?>
