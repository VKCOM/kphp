@unsupported
<?php
	class X
	{
		var $x;
		var $a = array ("x", "x", "x", "b");
		function x () { }
		function y () { return new X; }
	}

	function b ()
	{
		return new X ();
	}

	$y = "x";
	$z = "y";
	$x = new X ();
	$x->x = new X ();

	$x->x;
	$x->$y;
	$x->x();
	$x->$y();

	$x->$$z;

	$x->x->x;
	$x->$y->$y;
	$x->y()->x();
	$x->$z()->$y();

	$x->a[3];
	$x->a[3](); // Precedence inversion 
	$x->a[3]()->a[3]; // Precedence inversion
	$x->a[3]()->a[3](); // Precedence inversion (twice)

	$x->{2 + 3};
//	$x->{2 + 3}(); // moved to unsupported
?>
