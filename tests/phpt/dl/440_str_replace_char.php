@ok benchmark k2_skip
<?php

for ($i = 0; $i < 1000; $i++) {
  $s .= "a";
}
$t = $s."abacaba".$s;

for ($i = 0; $i < 20000; $i++) {
  $res += strlen (str_replace ("a", "testtest", $t));
}

var_dump ($res);
var_dump (substr (str_replace ("a", "testtest", $t), 0, 100));