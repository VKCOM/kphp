@kphp_should_fail
/isset, !==, ===, is_array or similar function result may differ from PHP/
<?php

function test() {
  [$x, $y] = explode(',', 'a,b', 2);
  if ($x === '') {}
}

test();
