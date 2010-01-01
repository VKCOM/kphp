@ok
<?php
	list($a, $b, $c) = array(1, 2, 3);
	list($d,   , $e) = array(4, 5, 6);

	#arseny30: nested lists are unsupported
	#list(list($f, $g), $h) = array(array(7, 8), 9);

	var_dump($a);
	var_dump($b);
	var_dump($c);
	var_dump($d);
	var_dump($e);
	#var_dump($f);
	#var_dump($g);
	#var_dump($h);
?>
