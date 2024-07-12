@ok k2_skip
<?php

$scalar_array = array (
  "",
  "a",
  "ab",
  "aasd",
  "really quite a really really long string, longer even",
  array (),
  array (17, 45),
  array (17 => "asd", 45 => ""),
  array ("ppp", "jjj"),
  array ("ppp" => 17, "jjj" => 45),
  array ("2", "3"),
  array ("2" => 2, "3" => 3),
  array (array ("rrr")),
  56,
  -17,
  0,
  1,
  -1,
  62.75,
  0.0,
  -0.0,
  NULL,
  false,
  true);

$short_scalar_array = array (
  "",
  "really quite a re",
  array ("2", 3, "four"),
  0,
  1,
  -1,
  62.75,
  0.0,
  -0.0,
  NULL,
  false,
  true);

echo "-----------<stage 1>-----------\n";

var_dump ($scalar_array);

echo "-----------<stage 2>-----------\n";

print_r ($short_scalar_array);

$x[] = 5;
var_dump ($x);


echo "-----------<stage 3>-----------\n";

foreach ($scalar_array as $scalar) {
  // skip string, and do them last
  if (is_string ($scalar) && strlen ($scalar) > 0)
    continue;

  echo "\nTrying: ";
  var_dump ($scalar);

  $x = $scalar;
#ifndef KPHP
  if (!is_string($x))
#endif
  @$x[] = 5;
  var_dump ($x);

//		$x = $scalar;
//		$y =& $x[];
//		$y = 6;
//		var_dump ($x);
}

echo "-----------<stage 4>-----------\n";

$x = "str";
//	$x[] = 5;
var_dump ($x);

?>
