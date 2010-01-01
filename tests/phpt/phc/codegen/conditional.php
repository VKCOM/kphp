@ok
<?php
	function f()
	{
		echo "this is f\n";
		return 1;
	}

	function g()
	{
		echo "this is g\n";
		return 2;
	}

	var_dump(true ? f() : g());
	var_dump(false ? f() : g());
?>
