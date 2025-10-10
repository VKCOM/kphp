@ok
<?php
  $a[] = acos (5);
  $a[] = acos (0);
  $a[] = acos (-1);
  $a[] = acos (1);
  $a[] = @1e10101;
  $a[] = @-1e1010101;
  $a[] = NAN;
  
  foreach ($a as $x) {
    echo ("is_nan      ($x) = ".is_nan ($x)."\n");
    echo ("is_infinite ($x) = ".is_infinite ($x)."\n");
    echo ("is_finite   ($x) = ".is_finite ($x)."\n");
  }
