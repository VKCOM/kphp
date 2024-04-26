@ok
<?php

function test_openssl_random_pseudo_bytes() {
  for ($i = -300; $i < 2048; $i += 273) {
  /** @var string */
  $res = "";
#ifndef KPHP
    try {
      $res = openssl_random_pseudo_bytes($i);
    } catch (Exception $e) {
      if ($i < 1) {
        print("error");
      } else {
        critical_error("unexpected exception");
      }
    }
#else
    $res = openssl_random_pseudo_bytes($i);
    if ($res === false && $i < 1) {
      print("error");
    }
#endif
    var_dump($res);
  }
}
