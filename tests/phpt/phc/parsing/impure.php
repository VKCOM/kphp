@ok
<?php
	$x = array ();
	/**
	 * @kphp-required
	 **/
/**
 * @kphp-required
 * @kphp-infer
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
