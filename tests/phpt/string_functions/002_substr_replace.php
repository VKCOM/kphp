@ok k2_skip
<?php

// test substr_replace with potentially incorrect arguments
// to check that it handles corner cases in the same way as PHP does

$strings = ['', '_', '__', 'example string'];
$ints = [0, -1, 1, -2, 2, -100, 100];

foreach ($strings as $s) {
  foreach ($strings as $replacement) {
    foreach ($ints as $start) {
      foreach ($ints as $length) {
        var_dump(substr_replace($s, $replacement, $start, $length));
        var_dump(substr_replace($s, $replacement, $start, strlen($s)));
        var_dump(substr_replace($s, $replacement, strlen($s), $length));
      }
    }
  }
}
