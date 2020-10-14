@ok
<?php

	// This is a very strange example, in which a data is changed by reading it.
	// The var_dump () before the line marked * will print a different result
	// than the line the *, even though the data in p0 is only read in
	// between. This is because the reference assignment creates an array from
	// NULL in the variable $p. This example shows that

	//		$y = $x[$p[0]];

	// cannot be translated into

	//		$t0 =& $p[0];
	//		$t1 =& $x[$t0];
	//		$y =& $t1;

/**
 * @param mixed $v
 */
	function f(&$v) {
	}

	// Note the change in $p
	$p = NULL;
	var_dump ($p);
	f ($p[0]);
	var_dump ($p);

  echo "\n";
	// It only happens with references
	$p = NULL;
	var_dump ($p);
	$t = $p[0];
	var_dump ($p);

  echo "\n";
	// The array doesnt need to be NULL, just missing the index.
	$p = array (1 => 17);
	var_dump ($p);
	f ($p[2]);
	var_dump ($p);

  echo "\n";
	// The array doesnt need to be NULL, just missing the index.
	$p = array (1 => "17");
	var_dump ($p);
	$t = $p[2];
	var_dump ($p);

  echo "\n";
	$x = array (4 => "d");
	var_dump ($x);
	$y = $x[$p[0]];
	var_dump ($x);
?>
