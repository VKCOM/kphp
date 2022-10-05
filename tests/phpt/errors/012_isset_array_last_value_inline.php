@kphp_should_fail
/isset, !==, ===, is_array or similar function result may differ from PHP/
<?php

/** @param int[] $xs */
function test($xs) {
  if (array_last_value($xs) === 0) {}
}

test([]);
