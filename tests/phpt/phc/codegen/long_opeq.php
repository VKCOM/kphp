@ok
<?php
/**
 * @kphp-infer
 * @param string $param
 * @return int
 */
	function f($param)
	{
		echo "f ($param) is called\n";
		return 1;
	}

#ifndef KPHP
  $_6 = f("six");
  $_3 = f("three");
  $_1 = f("one");
  $x[$_1][2][$_3][4][5][$_6] += 1;

  if (false) {
#endif
  if (strpos(get_engine_version(), "Clang") !== false) {
  	$x[f("six")][2][f("three")][4][5][f("one")] += 1;
  } else {
    $x[f("one")][2][f("three")][4][5][f("six")] += 1;
  }

#ifndef KPHP
  }
#endif
	var_dump($x);
?>
