@ok
<?php

function test_compress_levels() {
  var_dump(zstd_compress("foo bar baz", 23));

  for ($i = -100; $i < 23; ++$i) {
    echo "Level $i: ", base64_encode(zstd_compress(str_repeat("foo bar baz", 1000), $i)), "\n";
  }
}

function test_compress_dict() {
  var_dump(zstd_compress_dict("foo bar baz", ""));
  var_dump(zstd_compress_dict("foo bar baz", "foo"));
  var_dump(zstd_compress_dict("foo bar baz", "foo bar baz"));
}

test_compress_levels();
test_compress_dict();
