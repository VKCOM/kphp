@ok
<?php

/** @return string|false */
function get_random_bytes($num)  {
#ifndef KPHP
  try {
    $res = openssl_random_pseudo_bytes($num);
  } catch (\Throwable $e) {
    return false;
  }
  return $res;
#endif
  return openssl_random_pseudo_bytes($num);
}

function test_openssl_random_pseudo_bytes() {
  var_dump(get_random_bytes(-100) === false);
  var_dump(get_random_bytes(-1) === false);
  var_dump(get_random_bytes(0) === false);
  var_dump(get_random_bytes(1) !== false);
  var_dump(get_random_bytes(20) != get_random_bytes(20));
}

test_openssl_random_pseudo_bytes();
