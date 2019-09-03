@ok
<?php
  //arseny30: var_export replaced with print_r
  //arseny30: unset added
	$x = array(1 => "a", 2 => "b", 3 => "c", 4 => "d");
	foreach($x as $y)
	{
		echo "$y\n";
		break;
	}
	
	foreach($x as $y => $z)
	{
		echo "$y => $z\n";
	}
	foreach($x as &$yy)
	{
		print_r ($x);
		echo "$yy\n";
		print_r ($x);
	}
	
	unset ($y);

	foreach($x as $y => &$zz)
	{
		print_r ($x);
		echo "$y => $zz\n";
		print_r ($x);
	}
?>
