@unsupported
<?php
	function f($a, $b = 2, $c = 3)
	{
		var_dump($a);
		var_dump($b);
		var_dump($c);
	}

	function g($d = array(1,2))
	{
		var_dump($d);
	}

	function h($e = array(1,array(2 => 3, 'a' => 4), true,"hi",2.5))
	{
		var_dump($e);
	} 

	f(0);
	f(0,0);
	f(0,0,0);

	g();
	g(0);

	h();
	h(0);
?>
