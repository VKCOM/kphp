@ok k2_skip
<?php

require_once 'kphp_tester_include.php';


/**
 * @param tuple(int|null, string|null, false|null, int[]|null) $tup
 */
function test_tuple($tup) {
  var_dump($tup[0] ?? 123);
  var_dump($tup[1] ?? "foo");
  var_dump($tup[2] ?? null);
  var_dump($tup[3] ?? [1, 2, 3]);
  var_dump($tup[0] ?? $tup[1] ?? $tup[2] ?? $tup[3] ?? 42);
}

test_tuple(tuple(1, "bar", false, [2]));
test_tuple(tuple(null, null, null, null));
