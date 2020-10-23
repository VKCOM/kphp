@ok
<?php

	// it looks like reference assignments to lists aren't allowed

	// note, weak references
	list ($a, $b) = array ("0" => "x", "1" => "y");
	var_dump ($a);
	var_dump ($b);
	echo "-------------0----------------\n";

	// a list expression evaluates to its rvalue
	var_dump (list ($a, $b) = array (11, 12, 13));
	var_dump ($a);
		var_dump ($b);
	echo "-------------1----------------\n";

	// switching the values doesnt work. $a and $b are switched before the array is evaluated
	var_dump (list ($a, $b) = array ($b, $a));
	var_dump ($a);
	var_dump ($b);
	echo "-------------2----------------\n";
/*
	// note this one doesnt fill the list, only an array will do that
	var_dump (list ($a, $b) = 21, 22, 23);
	var_dump ($a);
	var_dump ($b);
	echo "-------------3----------------\n";
	list(list($x, $y), list($z, $w), $a) = $arr;
	var_dump ($a);
	var_dump ($b);
	echo "-------------6----------------\n";

	list(list($x, , $z), list($z, ,$v), $a) = $arr;
	var_dump ($a);
	var_dump ($b);
	echo "-------------7----------------\n";
*/
	$arr = array ( 1, array (4,5,6,7), array (8,9,"a", array ("b", "c")));

	list($x, $y, $z) = $arr;
	var_dump ($a);
	var_dump ($b);
	echo "-------------4----------------\n";

	list($x, , $z) = $arr;
	var_dump ($a);
	var_dump ($b);
	echo "-------------5----------------\n";

	// isnt this an interesting feature
	$x = list ($y, $z) = array (1,2);
	var_dump ($a);
	var_dump ($b);
?>
