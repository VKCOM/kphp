@ok
<?php

function test_openssl_random_pseudo_bytes() {
  $length = 1;
  $str = openssl_random_pseudo_bytes($length);
  var_dump(strlen($str));

  $length = 2048;
  $str = openssl_random_pseudo_bytes($length);
  var_dump(strlen($str));
}
