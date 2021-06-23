@kphp_should_fail
/pass mixed to argument \$x of ensure_string/
<?php

function ensure_string(string $x) {}

function test() {
  /** @var mixed $v */
  static $v = 10;
  if (is_string($v)) {
    ensure_string($v);
  }
}

test();
