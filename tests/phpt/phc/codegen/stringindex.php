@ok
<?php
	$arr = array("a" => 1, "b" => 2, "c" => 3);
	var_dump($arr);

	$x = $arr["a"];
	var_dump($x);
	
	$y = $arr["d"];
#ifndef KPHP
  $y = 0;
#endif
	var_dump($y);
?>
