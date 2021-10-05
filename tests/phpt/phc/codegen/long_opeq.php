@ok
<?php
/**
 * @param string $param
 * @return int
 */
	function f($param)
	{
		echo "f ($param) is called\n";
		return 1;
	}

  $x[f("one")][2][f("three")][4][5][f("six")] += 1;

	var_dump($x);
?>
