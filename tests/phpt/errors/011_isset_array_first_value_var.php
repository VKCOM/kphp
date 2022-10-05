@kphp_should_fail
/isset, !==, ===, is_array or similar function result may differ from PHP/
<?php

/** @param int[] $xs */
function test($xs) {
  $v = array_first_value($xs);
  if ($v === 0) {}
}

test([]);
