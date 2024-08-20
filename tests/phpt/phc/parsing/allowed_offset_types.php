@ok k2_skip
<?php

/**
 * @return int
 */
	function f () { return 7; }
#	class A {}

	// An array index is an AST_expression. Check all the types we think are allowed.
	// assignment
	$x = array (($z = 12) => 13);
	var_dump ($x);

	// unary op
	$x = array ((!$z) => 16);
	var_dump ($x);

	// bin op
	unset ($c);
	$x = array (($z + $c) => 17);
	var_dump ($x);

	// conditional_expr
	unset ($y);
	$x = array (($z == 0 ? $y : $z) => 18);
	var_dump ($x);

	// pre_op
	$x = array ((++$z) => 22);
	var_dump ($x);

	// post_op
	$x = array (($z++) => 23);
	var_dump ($x);


	// method_invocation
	$x = array (f() => 24);
	var_dump ($x);

	// literal
	$x = array (1 => 26, false => 27, "asd" => 28, 1.34 => 29);
	var_dump ($x);

	// cast
	$x = array (($z = (int)("5")) => 15);
	var_dump ($x);

	// ignore_errors
	$x = array (@$z => 19);
	var_dump ($x);

  define ('FOObar', 123124);

	// constant
	$x = array (FOObar => 20);
	var_dump ($x);
?>
