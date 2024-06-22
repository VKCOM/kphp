@ok
<?php

$x = "";
for ($i = 0; $i <= 100; $i++) {
  $x .= "abcdef";
}

$deflated = gzdeflate($x, 1);
$compressed = gzcompress($x, 1);
$encoded = gzencode($x, 1);

var_dump(crc32($deflated));
var_dump(crc32($compressed));
var_dump(crc32($encoded));

var_dump(gzinflate($deflated));
var_dump(gzuncompress($compressed));
var_dump(gzdecode($encoded));
