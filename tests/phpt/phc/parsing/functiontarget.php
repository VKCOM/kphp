@unsupported
<?php
	function foobar()
	{
		echo "foobar\n";
	}

	class X
	{
		static function foo()
		{
			echo "X::foo\n";
		}

		function bar()
		{
			echo "Y::bar\n";
		}
	}

	$y = new X();

	foobar();
	X::foo();
	$y->bar();
?>
