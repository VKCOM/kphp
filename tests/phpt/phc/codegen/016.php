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
  if (false) {
#endif
    if (strpos(get_engine_version(), "Clang") !== false) {
      $value = g();
      $a[f()] = $value;
    } else {
	    $a[f()] = g();
	  }
#ifndef KittenPHP
  }
#endif
	var_dump($a);
?>
