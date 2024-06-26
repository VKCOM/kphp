@ok
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
    // result of casting object to array key is an empty string
    $arr = [0 => "zero", "" => "empty_key", 42 => "answer"];
    ensure($arr[$a] === "empty_key");

    // just check that doesn't crash
    $a[1234] = 123;
  }
}

function main() {
  /** @var mixed */
  $a = to_mixed(new MyLongClassName());
  check($a);
}

main();
