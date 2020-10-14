@ok
<?php

require_once 'kphp_tester_include.php';


/**
 * @param shape(w:int[]|null, x:int|null, y:string|null, z:false|null) $s
 */
function test_shape($s) {
  var_dump($s["x"] ?? 123);
  var_dump($s["y"] ?? "foo");
  var_dump($s["z"] ?? null);
  var_dump($s["w"] ?? [1, 2, 3]);
  var_dump($s["x"] ?? $s["y"] ?? $s["z"] ?? $s["w"] ?? 42);
}

test_shape(shape(["x" => 1, "y" => "bar", "z" => false, "w" => [2]]));
test_shape(shape(["x" => null, "y" => null, "z" => null, "w" => null]));
