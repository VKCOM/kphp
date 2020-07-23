@ok
<?php
/**
 * @kphp-infer
 * @param int $n
 * @return int
 */
	function factorial($n)
	{
		if($n == 0)
			return 1;
		else
			return $n * factorial($n - 1); 
	}

	echo factorial(5);
?>
