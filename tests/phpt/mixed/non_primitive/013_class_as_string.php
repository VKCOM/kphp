@ok
<?php
require_once 'kphp_tester_include.php';

class MyLongClassName {
}

function ensure(bool $x) {
  if (!$x) {
    exit(1);
  }
}

/** @param mixed $a */
function check($a) {
#ifndef KPHP
    // fatal error in PHP
    return
#endif
    // warning in KPHP
    $b = $a; // to prevent smart cast
    if ($b instanceof MyLongClassName) {
        if ((string)$a != "MyLongClassName") {
          exit(1);
        }
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
}

main();
