@kphp_should_warn k2_skip
/expression \$x = array_merge\(\$x, <...>\) can be replaced with array_merge_into\(\$x, <...>\)/
/expression \$y\['hello'\] = array_merge\(\$y\['hello'\], <...>\) can be replaced with array_merge_into\(\$y\['hello'\], <...>\)/
/expression \$z\['x'\]\[\$y\['hello'\]\[0\]\] = array_merge\(\$z\['x'\]\[\$y\['hello'\]\[0\]\], <...>\) can be replaced with array_merge_into\(\$z\['x'\]\[\$y\['hello'\]\[0\]\], <...>\)/
/expression A::\$x = array_merge\(A::\$x, <...>\) can be replaced with array_merge_into\(A::\$x, <...>\)/
<?php

class A {
  public $x = [1, 2, 3];
}

/**
 * @kphp-warn-performance array-merge-into
 */
function test() {
  $x = [1, 2, 3 ,4];
  if (0) {
    $x = array_merge($x, [1]);
  }

  $y = ["hello" => ["world"]];
  if (0) {
    $y["hello"] = array_merge($y["hello"], ["gg"]);
  }

  $z = ["x" => ["y" => [1, 2, 3]]];
  if (0) {
    $z["x"][$y["hello"][0]] = array_merge($z["x"][$y["hello"][0]], [3]);
  }

  $a = new A;
  if (0) {
    $a->x = array_merge($a->x, [5]);
  }
}

test();
