@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class MyLongClassName {
}

function ensure(bool $x) {
  if (!$x) {
    exit(123);
  }
}

/** @param mixed $a */
function check($a) {
#ifndef KPHP
  // fatal error in PHP
  return;
#endif
  // but warning in KPHP
  $b = $a; // to prevent smart cast
  if ($b instanceof MyLongClassName) {
    // result of casting object to int is 1
    ensure((int) $a == 1);
    ensure($a + 41 == 42);
    ensure([5, 6, 7] + $a == 4);
  }
}

function main() {
  /** @var mixed */
  $a = to_mixed(new MyLongClassName());
  check($a);

  if (true) {
    $a = null;
  }
  check($a);

  $x = new MyLongClassName();
  $y = to_mixed($x);
  $a = [$y];
  array_push($a, to_mixed($x));
  array_push($a, $y);
  check($a[0]);
  check($a[1]);
  check($a[2]);
}

main();
