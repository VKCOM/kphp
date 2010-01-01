@unsupported
<?php
	$x = new C;				// Unparses as $x = new C() [note the brackets]
	$x = new C();
	$y = new D(1);
	$z = new E($x, $y);

	$a = new $x;
	$b = new $x();
	$c = new $x(1);
	$d = new $x($y, $z);

	$e = new $x->y;
	$f = new $x->y();
	$g = new $x->y(1);
	$h = new $x->y($y, $z);
	
	$e = new $x->y->z;
	$f = new $x->y->z();
	$g = new $x->y->z(1);
	$h = new $x->y->z($y, $z);

	/* with reference */

	$x =& new C;
	$x =& new C();
	$y =& new D(1);
	$z =& new E($x, $y);

	$a =& new $x;
	$b =& new $x();
	$c =& new $x(1);
	$d =& new $x($y, $z);

	$e =& new $x->y;
	$f =& new $x->y();
	$g =& new $x->y(1);
	$h =& new $x->y($y, $z);
	
	$e =& new $x->y->z;
	$f =& new $x->y->z();
	$g =& new $x->y->z(1);
	$h =& new $x->y->z($y, $z);
?>
