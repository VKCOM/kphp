@kphp_should_fail
/isset, !==, ===, is_array or similar function result may differ from PHP/
<?php

function test() {
  $s = '';
  if (preg_match('/x(\d+)/', 'hello x50', $m)) {
    $s = $m[1];
  }
  if ($s !== '') {
    var_dump($s);
  }
}

test();
