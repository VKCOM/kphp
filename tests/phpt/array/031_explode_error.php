@kphp_should_fail k2_skip
/isset, !==, ===, is_array or similar function result may differ from PHP/
/ExprNode explode\[\.\] at 031_explode_error\.php:7/
<?php

function test() {
  [$x] = explode(',', 'a,b');
  if ($x === '') {}
}

test();
