@ok
<?php

function test_compress_levels() {
  var_dump(zstd_compress("foo bar baz", 23));

  for ($i = -100; $i < 23; ++$i) {
    if ($i == 0) {
      continue;
    }
    echo "Level $i: ", base64_encode(zstd_compress(str_repeat("foo bar baz", 1000), $i)), "\n";
  }
}


// zstd 0.11.0 version starts to compress at 0 level, see
// https://pecl.php.net/package-info.php?package=zstd&version=0.11.0
function test_zero_compress_level() {
  $zero_level = 0;
  #ifndef KPHP
  if (version_compare(phpversion("zstd"), "0.11.0") < 0) {
    $zero_level = 1;
  }
  #endif
  echo "Level 0: ", base64_encode(zstd_compress(str_repeat("foo bar baz", 1000), $zero_level)), "\n";
}

function test_compress_dict() {
  var_dump(zstd_compress_dict("foo bar baz", ""));
  var_dump(zstd_compress_dict("foo bar baz", "foo"));
  var_dump(zstd_compress_dict("foo bar baz", "foo bar baz"));
}

test_compress_levels();
test_zero_compress_level();
test_compress_dict();
