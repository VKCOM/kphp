@kphp_should_fail
/isset, !==, ===, is_array or similar function result may differ from PHP/
<?php

function test() {
  $s = '';
  if (preg_match_all('/x(\d+)/', 'x hello x50', $m)) {
    $s = $m[0][0];
  }
  if ($s !== '') {
    var_dump($s);
  }
}

test();
