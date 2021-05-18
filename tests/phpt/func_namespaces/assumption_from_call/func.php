@ok
<?php

require_once __DIR__ . '/Foo.php';

function new_foo(): \A\B\C\Foo {
  return new \A\B\C\Foo();
}
