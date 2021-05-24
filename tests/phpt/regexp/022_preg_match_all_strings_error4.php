@kphp_should_fail
/isset, !==, ===, is_array or similar function result may differ from PHP/
<?php

function test() {
  $x = [''];
  if (preg_match_all('/x(\d+)/', 'x hello x50', $m)) {
    $x = $m[0];
  }
  if (is_array($x)) {
    var_dump($x);
  }
}

test();
