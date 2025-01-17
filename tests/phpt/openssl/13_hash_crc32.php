@ok benchmark
<?php

function test_hash_crc32() {
  $a = array("", "asdasd", "abacaba", "1", NULL, false, true, "asdasdasdasdasd42n3jb23jkb2k3vb2hj3v41hj 13hj j23hbr j42hb j42hb jh43b rjh1hb 12jb 3jh4 b32 b24");

  foreach ($a as $s) {
    var_dump (dechex(crc32($s)));
  }
  for ($i = 0; $i < 100; $i++)
    var_dump (dechex(crc32($i)));
}

test_hash_crc32();
