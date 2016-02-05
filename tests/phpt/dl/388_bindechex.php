@ok
<?php
  $a = array (0, 1, 2, 3, 10, 15, 37, 100, 1000, 10000, 10000000, 2000000000);
  foreach ($a as $n) {
    var_dump (decbin ($n));
    var_dump (bindec (decbin ($n)));
//    var_dump (bindec (decbin (-$n)));
    var_dump (dechex ($n));
    var_dump (hexdec (dechex ($n)));
//    var_dump (hexdec (dechex (-$n)));
    echo "\n";
  }