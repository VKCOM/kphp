@unsupported
<?php
	class C
	{
		var $x = "foo";
	}

	$c = new C();

	echo $c->x . "\n";
	echo "bar {$c->x} baz\n";
?>
