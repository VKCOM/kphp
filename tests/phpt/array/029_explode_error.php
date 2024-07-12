@kphp_should_fail k2_skip
/isset, !==, ===, is_array or similar function result may differ from PHP/
<?php

function test() {
  [$x, $y] = explode(',', 'a,b');
  if ($x === '') {}
}

test();
