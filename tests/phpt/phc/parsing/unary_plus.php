@ok
<?php
function f() {
	$x = +1;
	var_dump ($x);
	$x = +1.0;
	var_dump ($x);
	$x = + +1;
	var_dump ($x);
	$x = + -1;
	var_dump ($x);

	$x = -1;
	var_dump ($x);
	$x = - -1;
	var_dump ($x);
	$x = - - -1;
	var_dump ($x);
	$x = - - - -1;
	var_dump ($x);
	$x = - - - - -1;
	var_dump ($x);

	$x = -1;
	var_dump ($x);
	$x = +-1;
	var_dump ($x);
	$x = -+-1;
	var_dump ($x);
	$x = +-+-1;
	var_dump ($x);
	$x = -+-+-1;
	var_dump ($x);
}

f();


?>
