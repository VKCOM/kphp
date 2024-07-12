@ok k2_skip
KPHP_ERROR_ON_WARNINGS=1
<?php

class A {
  public $x = [1, 2, 3];
  public $y = [5, 6, 7];
}

/**
 * @kphp-warn-performance array-merge-into
 */
function test() {
  $x1 = [1, 2, 3 ,4];
  $x2 = [5, 6, 7];
  if (0) {
    $x1 = array_merge($x2, [1]);
  }

  $y = ["hello" => ["world"]];
  if (0) {
    $y["hello1"] = array_merge($y["hello2"], ["gg"]);
  }

  $z = ["x" => ["y" => [1, 2, 3]]];
  if (0) {
    $z["x1"][$y["hello"][0]] = array_merge($z["x2"][$y["hello"][0]], [3]);
    $z["x"][$y["hello"][1]] = array_merge($z["x"][$y["hello"][2]], [3]);
  }

  $a1 = new A;
  $a2 = new A;
  if (0) {
    $a1->x = array_merge($a2->x, [5]);
    $a1->x = array_merge($a1->y, [7]);
    $a1->x = array_merge($a2->y, [11]);
  }
}

test();
