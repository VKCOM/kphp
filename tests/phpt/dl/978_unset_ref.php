@unsupported
<?php

	function by_ref (&$param)
	{
		unset ($param);
	}

	$x = 7;
	by_ref ($x);
	var_dump ($x);

