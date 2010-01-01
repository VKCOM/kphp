@unsupported
<?php

	// AST_constants in a variery of positions
	class Foo 
	{ 
		const BAR = 7;
		const Bar = 8;

		var $x1 = Foo::Bar;
		var $x2 = array (Foo::Bar);
		const X3 = Foo::Bar;
		static $x4 = Foo::Bar;
	}

	function x ($x = Foo::bar, $y = array ("s", Foo))
	{
		var_dump ($x);
		var_dump ($y);
	}

	x ();
	x (new Foo ());
	x (new Foo (), new Foo ());

?>
