@ok
<?php
  $a = 123;
  echo "$a\n";
  require "988_require_f.php";
  require_once "988_require_f.php";
  require "988_require_f.php";
  echo "$a\n";
  function f() {
    $a = 2;
    echo $a;
  }
  echo "hello world\n";
  $tmp = require "988_require_f.php";
  var_dump ($tmp);
?>
