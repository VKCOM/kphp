@kphp_should_warn
/variable \$a1 is implicitly converted from int\[\] to mixed\[\]/
/variable \$b1 is implicitly converted from string\[\] to mixed\[\]/
/variable \$a2 is implicitly converted from int\[\] to mixed/
/variable \$b2 is implicitly converted from string\[\] to mixed/
/variable \$a3 is implicitly converted from int\[\] to mixed/
/variable \$b3 is implicitly converted from string\[\] to mixed/
/variable \$a4 is implicitly converted from int\[\] to mixed/
/variable \$b4 is implicitly converted from string\[\] to mixed/
/variable A::\$b is implicitly converted from int\[\] to mixed\[\]/
/expresion <...> is implicitly converted from tuple\(int, int\[\]\) to tuple\(float, mixed\[\]\)/
<?php

class A {
    /** @var tuple(float, mixed[]) */
    public $a;
    public $b = [1, 2, 3, 4];
}

function call_with_implicit_array_cast(array $x) {
  var_dump($x);
}

/**
 * @kphp-warn-performance implicit-array-cast
 */
function test() {
  $a1 = [1, 2, 3, 4];
  call_with_implicit_array_cast($a1);

  $b1 = ["hello"];
  call_with_implicit_array_cast($b1);

  $a2 = $a1;
  $b2 = $b1;

  $c = 1 ? [1] : "x";
  $c[] = $a2;
  $c["xx"] = $b2;

  $a3 = $a1;
  $b3 = $b1;
  $c[] = [$a3, $b3];

  $a4 = $a1;
  $b4 = $b1;
  if (0) {
    $c = $a4;
    if (0) {
      return $b4;
    }
  }

  $a = new A;
  call_with_implicit_array_cast($a->b);
  $a->a = tuple(4, [1, 2, 3]);

  return $c;
}

test();
