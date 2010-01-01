@unsupported
<?php

require ("scalar_array.php");

class A
{
	// the PHP docs say an object converts to the string "Object", but it doesnt
	// look like it
	function __toString()
	{
		return "a";
	}
}


class B
{
	function __construct ()
	{
		$this->f = new stdClass;
		$this->f->g = new stdClass;
		$this->f->g->h = array ();
	}
}

$scalar_array[] = new A ();
$scalar_array[] = new B ();

foreach ($scalar_array as $y)
{
	print "Converting: ";
	var_dump ($y);

	print "Converting to an int: ";
	$x = (int) $y;
	var_dump ($x);

	print "Converting to an integer: ";
	$x = (integer) $y;
	var_dump ($x);

	print "Converting to an float: ";
	$x = (float) $y;
	var_dump ($x);

	print "Converting to an real: ";
	$x = (real) $y;
	var_dump ($x);

	print "Converting to an double: ";
	$x = (double) $y;
	var_dump ($x);

	print "Converting to an string: ";
	$x = (string) $y;
	var_dump ($x);

	print "Converting to an array: ";
	$x = (array) $y;
	var_dump ($x);

	print "Converting to an object: ";
	$x = (object) $y;
	var_dump ($x);

	print "Converting to an bool: ";
	$x = (bool) $y;
	var_dump ($x);

	print "Converting to an boolean: ";
	$x = (boolean) $y;
	var_dump ($x);

	print "Converting to an unset: ";
	$x = (unset) $y;
	var_dump ($x);

	# some variations

	print "Converting to an BOOLean: ";
	$x = (BOOLean) $y;
	var_dump ($x);

	$x = (       string     ) $y;
	var_dump ($x);

	$x = (				string) $y;
	var_dump ($x);

	print "\n\n";
}
?>
