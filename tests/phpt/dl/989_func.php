@ok
<?php
  function f($a, $b = 2, $c = 3) {
    echo ("$a $b $c\n");
  }
  f (1);
  f (1, 5);
  f (1, 5, 7);
//  f ();
?>
