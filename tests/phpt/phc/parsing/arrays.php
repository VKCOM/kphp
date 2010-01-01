@ok
<?php
  $y = null;
  $x = array(array(1));
	$x[0];
	$x[0][0];
//	$x[]; // moved to unsupported
//	$x[][];
//	$x[0][];
//	$x[][0];

  //unpported
	#${$x[0]};

	$a1 = array('a');
	$a2 = array($x);
	$a3 = array(1 => 'a');
	$a4 = array(1 => $x); 
	
	$a5 = array('a', 'b', 'c');
	$a6 = array($x, $y, $y);
	$a7 = array(1 => 'a', 2 => 'b', 3 => 'c');
	$a8 = array(1 => $x, 2 => $x, 3 => $x);

	var_dump ($a1);
	var_dump ($a2);
	var_dump ($a3);
	var_dump ($a4);
	var_dump ($a5);
	var_dump ($a6);
	var_dump ($a7);
	var_dump ($a8);
?>
