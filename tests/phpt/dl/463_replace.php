@ok
<?php
  $x = false;
  $x = "/a/";
  $count = 0;
  var_dump (preg_replace ($x, "aba", "abacaba", 3, $count));
  var_dump ($count);
  var_dump (preg_replace ($x, "aba", "abacaba", 3, $count));
  var_dump ($count);
