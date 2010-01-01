@unsupported
<?php
	// list assignment (list assignments are generally to arrays, so it would
	// evaluate to an array. But not here.)
	$x = array ((list ($z) = 6) => 14);
	var_dump ($x);

	// instanceof
	$x = array (($z instanceof int) => 21);
	var_dump ($x);

	// array - not allowed
//	$x = array (array () => 24);
//	var_dump ($x);

	// new - not allowed
//	$x = array (new A () => 24);
//	var_dump ($x);

	// clone - clones objects only, so not allowed
//	$x = array (clone ($z) => 25);
//	var_dump ($x);

?>
