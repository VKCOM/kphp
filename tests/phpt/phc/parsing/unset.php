@unsupported
<?php
	unset($x, $a->b);

	var_dump ($x, $a);
	// unset(f());

	$x = (unset) $x;
?>
