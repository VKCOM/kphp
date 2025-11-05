@ok
<?php

require_once 'kphp_tester_include.php';

function test_compress_uncompress() {
  var_dump(zstd_uncompress((string)zstd_compress("foo bar baz")));

  var_dump(zstd_uncompress((string)zstd_compress(str_repeat("foo bar baz", 10000))));

  $random_data = (string)openssl_random_pseudo_bytes(1024*1024*15);
  assert_true(zstd_uncompress((string)zstd_compress($random_data)) === $random_data);
}


test_compress_uncompress();
