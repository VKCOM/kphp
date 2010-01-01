@unsupported
<?php

function foobar()
{
	echo "foobar()\n";
}

class Foo
{

	function foobar()
	{
		echo "Foo::foobar()\n";
	}

	function Foo()
	{
		$this->foobar();
		foobar();
	}

}

$x = new Foo();
foobar();

?>
