@ok
<?php

function is_kphp() {
#ifndef KPHP
    return false;
#endif
    return true;
}

function test_openssl_random_pseudo_bytes() {
  for ($i = -300; $i < 2048; $i += 273) {
    $res = "";

    if (is_kphp()) {
      $res = openssl_random_pseudo_bytes($i);
      if ($res === false && $i < 1) {
        print("error");
      }
    } else {
      try {
        $res = openssl_random_pseudo_bytes($i);
      } catch (Exception $e) {
        if ($i < 1) {
          print("error");
        } else {
          critical_error("unexpected exception");
        }
      }
    }
    var_dump($res);
  }
}
