@ok no_php
<?php
	$x = array(1 => "a", 2 => "b", 3 => "c", 4 => "d");
	foreach($x as &$y)
	{
		print_r ($x);
		echo "$y\n";
		print_r ($x);
	}
	
	foreach($x as $y => &$z)
	{
		print_r ($x);
		echo "$y => $z\n";
		print_r ($x);
	}
?>
