@ok k2_skip
<?php

/** @param mixed $x */
function test(string $s, $x) {
  var_dump(strip_tags($s, $x));
}

test('<hr><b>hello</b><br/>', ['b']);
test('<hr><b>hello</b><br/>', ['br', 'b']);
test('<hr><b>hello</b><br/>', '<b>');
test('<hr><b>hello</b><br/>', '<br><b>');
