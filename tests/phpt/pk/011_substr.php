@ok k2_skip
<?php

$str = "abcde";

for ($i = -10; $i <= 10; $i++) {
  for ($j = -10; $j <= 10; $j++) {
    echo $i . " " . $j . "\n";
    var_dump(substr($str, $i, $j)); 
  }
  echo $i . "\n";
  var_dump(substr($str, $i)); 
}
