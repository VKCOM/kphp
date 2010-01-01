@unsupported
<?php
	class Foo
	{
		static $x1;
		  
		static $x2 = 10;
		static $x3 = FOO;
		static $x4 = FOO::bar;
	
		static $x5, $y;
	
		static $x6 = array(1,2,3);
		static $x7 = array(1 => 2, 2 => 3);
		static $x8 = array(FOO => BAR, FOOBAR);

		static $x9 = +5;
		static $x10 = -5;
	}

	$y = Foo::$x1;
?>
