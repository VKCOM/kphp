@ok
<?php

$x = array(1 => "a", 2 => "b", 3 => "c", 4 => "d");

echo "-----------<stage 1>-----------\n";

foreach($x as &$y) {
  print_r ($x);
  echo "$y\n";
  print_r ($x);
}

echo "-----------<stage 2>-----------\n";

foreach($x as $y1 => &$z) {
  print_r ($x);
  echo "$y1 => $z\n";
  print_r ($x);
}
