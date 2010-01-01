@ok
<?php
	function f()
	{
		echo "this is f\n";
		return false;
	}

	function t()
	{
		echo "this is t\n";
		return true;
	}

	var_dump(f() && f());
	var_dump(f() && t());
	var_dump(t() && f());
	var_dump(t() && t());

	var_dump(f() || f());
	var_dump(f() || t());
	var_dump(t() || f());
	var_dump(t() || t());
?>
