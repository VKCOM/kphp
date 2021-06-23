@kphp_should_fail php7_4
/returned nullable Foo value from test/
<?php

function new_nullable_foo(): ?Foo {
  return new Foo();
}

function test(): Foo {
  /** @var ?Foo */
  $foo = null;
  if (!$foo) {
    $foo = new_nullable_foo();
  }
  return $foo;
}

class Foo {}

test();
