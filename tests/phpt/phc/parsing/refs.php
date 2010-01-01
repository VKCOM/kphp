@unsupported
<?php
	/*
	 * Test to confirm the semantics of the & operator in various contexts
	 *
	 * phc -- The PHP compiler
	 */

	/*
	 * assignment ::= variable '&'? expr
	 */

	$base = 1;
	$ref =& $base;

	var_dump($base);		// int(1) 
	var_dump($ref);		// int(1)

	$ref = 2;

	var_dump($base);		// int(2)
	var_dump($ref);		// int(2);

	/*
	 * array_elem ::= expr? '&' expr
	 */

	$base_1 = 3;
	$base_2 = 4;

	$ref = array(1 => &$base_1, 2 => &$base_2);

	var_dump($ref);		// [1]=>&int(3) [2]=>&int(4)

	$base_1 = 5;
	$base_2 = 6;

	var_dump($ref);		// [1]=>&int(5)	[2]=>&int(6)
	
	$ref[1] = 7;
	$ref[2] = 8;

 	var_dump($ref);  		// [1]=>&int(7) [2]=>&int(8)
	var_dump($base_1);	// int(7)
	var_dump($base_2);	// int(8)

	$base_3 = 9;
	$ref[2] =& $base_3;

	var_dump($ref);		// [1]=>&int(7) [2]=>&int(9)

	/*
	 * foreach ::= expr VARIABLE '&' VARIABLE statement*
	 *
	 * Note the difference in the structure after the second foreach
	 * (I do not know whether this is something we need to mimick)
	 */

	$arr = array(1 => 11, 2 => 12);
	var_dump($arr);		// [1]=> int(11) [2]=> int(12)

	foreach($arr as $key => $val) { $key += 10; $val *= 2; }
	var_dump($arr);		// [1]=> int(11) [2]=> int(12)

	foreach($arr as $key => &$val) { $key += 10; $val *= 2; }
	var_dump($arr);		// [1]=> &int(22) [2]=> &int(24)
	
	foreach($arr as $key => &$val) { $key += 10; $val *= 2; }
	var_dump($arr);		// [1]=> &int(44) [2]=> &int(48)

	/*
	 * formal_parameter ::= IDENT? '&'? VARIABLE static_scalar?
	 *
	 * Note that the global keyword overrides the & in function f3.
	 */

	$x = 2;
	
	function f1($par)
	{
		var_dump($par);
		$par *= 2;
	}

	f1($x);					// int(2)
	var_dump($x);			// int(2)

	function f2(&$par)
	{
		var_dump($par);
		$par *= 2;
	}

	f2($x);					// int(2)
	var_dump($x);			// int(4)

	function f3(&$par)
	{
		global $par;
		var_dump($par);
		$par *= 2;
	}

	f3($x);					// NULL
	var_dump($x);			// int(4)

	/*
	 * signature ::= '&'? IDENT formal_parameter*
	 *
	 * Note the difference between the lines marked [*] and [**]
	 */
	
	function f4()
	{
		static $in_f4 = 0;

		$in_f4++;
		return $in_f4;
	}

	$out_f4 = f4();
	var_dump($out_f4);		// int(1)

	$out_f4 = f4();
	var_dump($out_f4);		// int(2)

	$out_f4 = 10;
	var_dump($out_f4);		// int(10)
	$out_f4 = f4();
	var_dump($out_f4);		// int(3)

	$out_f4 =& f4();
	var_dump($out_f4);		// int(4)
	$out_f4 = 10;
	var_dump($out_f4);		// int(10)
	$out_f4 = f4();
	var_dump($out_f4);		// int(5) [*]
	
	function &f5()
	{
		static $in_f5 = 0;

		$in_f5++;
		return $in_f5;
	}

	$out_f5 = f5();
	var_dump($out_f5);		// int(1)

	$out_f5 = f5();
	var_dump($out_f5);		// int(2)

	$out_f5 = 10;
	var_dump($out_f5);		// int(10)
	$out_f5 = f5();
	var_dump($out_f5);		// int(3)

	$out_f5 =& f5();
	var_dump($out_f5);		// int(4)
	$out_f5 = 10;
	var_dump($out_f5);		// int(10)
	$out_f5 = f5();
	var_dump($out_f5);		// int(11) [**]

	/*
	 * actual_paramter ::= '&' VARIABLE
	 *
	 * Note the discrepency between this and the previous test. 
	 * - If a function _returns_ a symbol table entry, the associated assignment
	 *   must also be =& (and not just =; cf. test above)
	 * - For a function to _accept_ a symbol table entry, there is a choice:
	 *   either you add the & to the formal parameter, or you add the & to the
	 *   actual parameter (both is allowed too). 
	 */
	
	function h1($in_h1)
	{
		$in_h1++;
	}

	$out_h1 = 5;
	h1(&$out_h1);
	var_dump($out_h1);			// Outputs 6	
	
	function h2(&$in_h2)
	{
		$in_h2++;
	}

	$out_h2 = 5;
	h2(&$out_h2);
	var_dump($out_h2);			// Outputs 6	
?>
