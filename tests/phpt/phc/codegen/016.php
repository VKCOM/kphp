@ok
<?php
/**
 * @kphp-infer
 * @return int
 */
	function f() {
		echo "called f\n";
		return 1;
	}
	
/**
 * @kphp-infer
 * @return int
 */
	function g() {
		echo "called g\n";
		return 2;
	}

#ifndef KPHP
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
#ifndef KPHP
  }
#endif
	var_dump($a);
?>
