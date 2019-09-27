@ok
<?php
	if(1)
	{
		function f()
		{
			echo "first f\n";
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
