@ok
<?php
/**
 * @kphp-infer
 * @return false
 */
	function f()
	{
		echo "this is f\n";
		return false;
	}

/**
 * @kphp-infer
 * @return bool
 */
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
