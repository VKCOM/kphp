@ok
<?php

function test_array_lt() {
  var_dump([] < []);
  var_dump([1] < []);
  var_dump([] < [1]);
  var_dump([1] < [1]);
  var_dump([1] < [2]);
  var_dump([1, 2] < [2]);
  var_dump([1] < [1, 2]);
  var_dump([1, 2] < [1, 2]);
  var_dump([1, 2] < [2, 1]);
  var_dump([2, 1] < [2, 1]);
  var_dump([2, 1] < [1, 2]);
}

test_array_lt();
