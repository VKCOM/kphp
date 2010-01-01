@unsupported
<?php
	class C
	{
		var $d;
	}

	$x = 0;
	$c = new C();

	function &f1()
	{
		global $x;
		return $x;
	}

	function f2()
	{
		return "x";
	}

	function f3()
	{
		/*
		 * Global requires a variable_name
		 */
		 
		// global f1();						// Illegal
		global ${f2()};						// Legal
		// global $c->d;						// Illegal
	}

	/*
	 * Unset takes an arbitrary expression (in particular, invocation)
	 */

	// unset(f());								// Illegal
	unset(${f2()});							// Legal
	unset($c->d);								// Legal

	/*
	 * Foreach
	 */
	
	$arr = array(1 => 'a', 2 => 'b', 3 => 'c'); 
	
	// foreach($arr as $y => f1()) {}	// Illegal
	foreach($arr as $y => ${f2()})		// Legal 
	{
		echo "$x\n";
	}
	foreach($arr as $y => $c->d)			// Legal
	{
		echo "$c->d\n";
	}
	
	// foreach($arr as f1() => $y) {}	// Illegal
	foreach($arr as ${f2()} => $y)		// Legal 
	{
		echo "$x\n";
	}
	foreach($arr as $c->d => $y)			// Legal
	{
		echo "$c->d\n";
	}
?>
