@ok
<?php
require_once 'kphp_tester_include.php';

class A {
public int $field;
}

/** @param $x mixed */
function print_mixed($x) {
#ifndef KPHP
  echo "string(1) \"A\"\n";
  return;
#endif
  var_dump($x);
}

/** @var mixed */
$x = to_mixed(new A);

print_mixed($x);
