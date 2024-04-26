@ok
<?php

function is_kphp() {
#ifndef KPHP
    return false;
#endif
    return true;
}

function test_xor_strings() {
  $strings = ["handling and manipulating", "ings, take a look", "ctory Access Protocol", "0000000dwxasd", "1111sadsd11"];

  foreach ($strings as $str1) {
    foreach($strings as $str2) {
      if (is_kphp()) {
        $res = xor_strings($str1, $str2);
      } else {
        $res = $str1 ^ $str2;
      }
      var_dump($res);
    }
  }
}
