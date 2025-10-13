@ok benchmark k2_skip
<?php
  $start = 1;
  $finish = 350000;

  for ($i = $start; $i < $finish; $i++) {
    $a[$i] = 1;
  }

  for ($i = $start; $i < $finish; $i++) {
    unset ($a[$i]);
  }
