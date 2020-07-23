@ok
<?php
/**
 * @kphp-infer
 * @return int
 */
	function f()
	{
		echo "f was called\n";
		return 0;
	}

	$a[0] = 1;
	$a[f()]++;
	++$a[f()];
	var_dump($a[0]);
?>
