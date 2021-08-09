@ok
<?php

function test_list() {
  $x = 10;
  $xs = [1, 2, 3];

  // $x is not captured here as it's part of the list()
  $f = fn() => list($x) = $xs;

  var_dump($x, $f());
}

test_list();
