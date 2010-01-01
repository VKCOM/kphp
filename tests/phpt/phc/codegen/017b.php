@ok
<?php
	$x["a"] = 1;
	$x["b"] = 2;
	$x["c"] = 3;
	unset($x["b"]);
	var_dump($x);
?>
