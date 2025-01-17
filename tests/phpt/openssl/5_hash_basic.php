@ok benchmark
<?php

function test_hash_basic() {
  hash_algos();
  $algos = array ("md5", "sha1", "sha224", "sha256", "sha384", "sha512");

  $a = array("", "asdasd", "abacaba", "1", NULL, false, true, "asdasdasdasdasd42n3jb23jkb2k3vb2hj3v41hj 13hj j23hbr j42hb j42hb jh43b rjh1hb 12jb 3jh4 b32 b24");

  foreach ($a as $s) {
    var_dump (sha1 ($s, false));
    var_dump (sha1 ($s, true));
    var_dump (sha1 ($s));
    var_dump (md5 ($s, false));
    var_dump (md5 ($s, true));
    var_dump (md5 ($s));

    foreach ($algos as $a) {
      var_dump (hash_hmac ($a, $s, $s, true));
      var_dump (hash_hmac ($a, $s, $s));
      var_dump (hash_hmac ($a, $s, "dummy", true));
      var_dump (hash_hmac ($a, $s, "dummy"));
      var_dump (hash_hmac ($a, "dummy", $s, true));
      var_dump (hash_hmac ($a, "dummy", $s));

      var_dump (hash ($a, $s, true));
      var_dump (hash ($a, $s, false));
    }
  }
  for ($i = 0; $i < 100; $i++)
    var_dump (md5 ($i));
}

function test_hash_equals() {
  var_dump(hash_equals("", ""));

  var_dump(hash_equals("xx", "x"));
  var_dump(hash_equals("x", "xx"));
  var_dump(hash_equals("x", "y"));
  var_dump(hash_equals("hello", "world"));
  var_dump(hash_equals("1", "12"));
  var_dump(hash_equals("1234", "2345"));

  var_dump(hash_equals("xx", "xx"));
  var_dump(hash_equals("xyz", "xyz"));
  var_dump(hash_equals("hello world", "hello world"));
  var_dump(hash_equals("12345", "12345"));
}


test_hash_basic();
test_hash_equals();
