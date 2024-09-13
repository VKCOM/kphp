@ok k2_skip
<?php
  ob_start();
  echo "1:blah\n";
  ob_start();
  echo "2:blah";
  var_dump(ob_get_clean());
  var_dump("0");
  var_dump(ob_get_clean());
  var_dump("0");
