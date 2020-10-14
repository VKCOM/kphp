@ok
<?php

	// Commented statements arent allowed by the parser

	// compare print and echo
	$z = 2;
/**
 * @return string
 */
	function f () 
	{
		global $z;
		$z++;
		return "--$z--\n";
	}
	$x = "15\n";

//	print;
//	print ();
//	echo;

	print $x;
	echo $x;

	print f ();
	echo f ();

//	$y = echo 5;
	$y = print 5;

	echo f(), $x, f(); 
//	print f(), $x, f();

	print print print $x;
//	echo echo echo $x;
	echo print print $x;
//	print echo $x;
	
	echo f(), print print $x, $x;

//	echo &f();
//	print &f();

//	echo &$x;
//	print &$x;

//	$y = &echo f ();
//	$y = &print f ();

	// we convert print to printf, which returns the length of the string.
	// Print, however, always returns 1.
	print print "a string of some length";

	// This might fail if we moved print before we shredded.
/**
 * @return int
 */
	function x ()
	{
		return print "a string longer than 1";
	}

	print x ();

?>
