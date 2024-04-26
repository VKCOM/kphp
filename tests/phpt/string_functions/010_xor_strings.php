@ok
<?php

function test_xor_strings() {
  $strings = ["handling and manipulating", "ings, take a look", "ctory Access Protocol", "0000000dwxasd", "1111sadsd11"];

  foreach ($strings as $str1) {
    foreach($strings as $str2) {
#ifndef KPHP
        $res = $str1 ^ $str2;
#else
        $res = xor_strings($str1, $str2);
#endif

        var_dump($res);
    }
  }
}
