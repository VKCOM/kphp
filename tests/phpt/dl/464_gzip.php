@ok k2_skip
<?php
#ifndef KPHP
if(!function_exists('gzdecode')){
function gzdecode($string) {
  $string = substr($string, 10);
  return gzinflate($string);
}
}
#endif


function test_basic() {
  $s = gzcompress("Hello world");

  $f = fopen("test.gz", "w");
  fwrite($f, $s);
  fclose($f);

  var_dump(gzuncompress(file_get_contents("test.gz")));

  $s = gzencode("Hello world");

  $f = fopen("test.gz", "w");
  fwrite($f, $s);
  fclose($f);

  var_dump(gzdecode(file_get_contents("test.gz")));

  unlink("test.gz");
}

function test_large_string() {
  $s = str_repeat("x", 1 + 8*1024*1024);
  $compressed = gzcompress($s);
  var_dump(strlen($compressed));

  $unc = gzuncompress($compressed);
#ifndef KPHP
  $unc = "";
#endif
  var_dump($unc);
}

test_basic();
test_large_string();
