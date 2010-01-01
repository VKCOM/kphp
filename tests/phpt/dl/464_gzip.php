@ok
<?php
#ifndef KittenPHP
function gzdecode($string) {
  $string = substr($string, 10);
  return gzinflate($string);
}
#endif
  $s = gzcompress ("Hello world");

  $f = fopen ("test.gz", "w");
  fwrite ($f, $s);
  fclose ($f);

  var_dump (gzuncompress (file_get_contents ("test.gz")));

  $s = gzencode ("Hello world");

  $f = fopen ("test.gz", "w");
  fwrite ($f, $s);
  fclose ($f);

  var_dump (gzdecode (file_get_contents ("test.gz")));

  unlink ("test.gz");
