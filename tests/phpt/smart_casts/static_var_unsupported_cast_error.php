@kphp_should_fail
/pass mixed to argument \$x of ensure_int/
<?php

function ensure_int(int $x) {}

function test() {
  /** @var mixed $v */
  static $v = 10;
  if (is_int($v)) {
    ensure_int($v);
  }
}

test();
