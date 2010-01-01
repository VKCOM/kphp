@unsupported
<?php

	$x = 100;

	function foo()
	{
		$x = 400;
		$y = 500;

		global $x;

		$x = 600;
		$y = 700;
	}

	$y = 200;

	class Fud
	{
		var $x;

		function fudbar()
		{
			$x = 800;

			global $x;

			$x = 900;
		}
	}

?>
