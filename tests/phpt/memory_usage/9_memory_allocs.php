@ok k2_skip
<?php

function f(int $x) {
#ifndef KPHP
  var_dump(1);
  return [$x];
#endif
  [$num_allocs, ] = memory_get_allocations();
  $arr = [$x];
  [$new_num_allocs, ] = memory_get_allocations();
  var_dump($new_num_allocs - $num_allocs);
  return $arr;
}

var_dump(f(0));
