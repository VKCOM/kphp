@unsupported
<?php

$x = 10;
$y = 20;

function foo()
{

	$y = "y";
	global $x, $$y;
		  
}

foo();

?>
