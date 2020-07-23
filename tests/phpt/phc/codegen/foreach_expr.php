@ok
<?php

	// this tests that the expression in a foreach loop is not
	// executed more than once
	$arr = array ();
	$a = 0;
	$a1 = 0;
/**
 * @kphp-infer
 * @param mixed $x
 * @param mixed $y
 * @return int[]
 */
	function f ($x = 0, $y = 0)
	{
		global $arr;
		global $a;
		$arr[] = $a++;
		return $arr;
	}

/**
 * @kphp-infer
 * @param int $x
 * @param int $y
 * @return int[]
 */
	function g ($x = 0, $y = 0)
	{
		global $arr2;
		global $a1;
		$arr2[] = $a1++;
		return $arr2;
	}

	for ($i = 0; $i < 5; $i++)
	{
		foreach (f("1", g()) as $x)
		{
			echo $x;
		}
	}

?>
