@ok
<?php
	function f() {
		echo "called f\n";
		return 1;
	}
	
	function g() {
		echo "called g\n";
		return 2;
	}

#ifndef KittenPHP
  $value = g();
	$a[f()] = $value;
  if (false)
#endif
	$a[f()] = g();
	var_dump($a);
?>
