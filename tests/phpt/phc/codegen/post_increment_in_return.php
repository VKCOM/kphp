@ok UB
<?php
	$i = 0;

	function f()
	{
		global $i;
		return $i++ * 2;
	}

	var_dump(f() + $i);
	var_dump(f() + $i);
	var_dump(f() + $i);
?>
