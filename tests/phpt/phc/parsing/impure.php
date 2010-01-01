@unsupported
<?php

	/* The purity checker hasnt been working for a while now. Test it. This should fail */
	$x = array ();
	function f ($element1, $element2)
	{
		return $element1;
	}

	$y = array_reduce ($x, f);

?>
