@ok k2_skip
<?php
    /** @var int[] */
	$x = array ();
/**
 * @kphp-required
 * @param null $element1
 * @param any $element2
 * @return null
 */
	function f ($element1, $element2)
	{
		return $element1;
	}

	$y = array_reduce ($x, "f", null);

?>
