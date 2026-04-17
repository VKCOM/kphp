@ok
<?php

function test_auto_memory_defragmentation() {
  $a = "a";
  $b = "b";

  $n = 5000000;
#ifndef K2
  $n = 10000000;
#endif
  for ($i = 1; $i < $n; ++$i) {
    $b .= "b";
    $a .= "a";
  }

#ifndef KPHP
  var_dump(0);
  if (false)
#endif
  var_dump(memory_get_detailed_stats()["defragmentation_calls"]);

  $n *= 2;
#ifndef K2
  $n *= 5;
#endif
  $a = "a";
  $b = "b";
  for ($i = 1; $i < $n; ++$i) {
    $b .= "b";
    $a .= "a";
  }

#ifndef KPHP
  var_dump(1);
  if (false)
#endif
  var_dump(memory_get_detailed_stats()["defragmentation_calls"]);
}

test_auto_memory_defragmentation();
