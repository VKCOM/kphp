@kphp_should_fail k2_skip
/isset, !==, ===, is_array or similar function result may differ from PHP/
/ExprNode explode\[\.\] at 028_explode_error\.php:7/
<?php

function test() {
  $s = explode(',', 'a,b')[0];
  if ($s === null) {}
}

test();
