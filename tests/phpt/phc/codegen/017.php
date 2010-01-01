@ok
<?php
	$x[1] = 1;
	$x[2] = 2;
	$x[3] = 3;
	unset($x[2]);
	var_dump($x);
?>
