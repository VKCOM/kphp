@unsupported
<?php
	if($expr)
	{
		function f()
		{
			echo "first f\n";
		}
	}
	else
	{
		function f()
		{
			echo "second f\n";
		}
	}

	f();

	function g()
	{
		function h()
		{
			echo "h\n";
		}
	}

	g();
	h();
?>
