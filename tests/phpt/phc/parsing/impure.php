@ok
<?php
	$x = array ();
	/**
	 * @kphp-required
	 **/
	function f ($element1, $element2)
	{
		return $element1;
	}

	$y = array_reduce ($x, "f", null);

?>
