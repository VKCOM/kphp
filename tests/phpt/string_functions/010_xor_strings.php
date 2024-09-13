@ok
<?php

function xor_impl(string $s1, string $s2) {
#ifndef KPHP
    return $s1 ^ $s2;
#endif
    return xor_strings($s1, $s2);
}

function test_xor_strings() {
  $strings = ["handling and manipulating", "ings, take a look", "ctory Access Protocol", "0000000dwxasd", "1111sadsd11", "a", "-"];

  foreach ($strings as $str1) {
    foreach($strings as $str2) {
      var_dump(xor_impl($str1, $str2));
    }
  }
}

test_xor_strings();
