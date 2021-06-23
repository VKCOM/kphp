@kphp_should_fail
/passed nullable Foo value as \$foo argument to nonnull_foo/
<?php

function new_nullable_foo(): ?Foo {
  return new Foo();
}

function test() {
  /** @var ?Foo */
  $foo = null;
  if (!$foo) {
    $foo = new Foo();
  }
  $foo = new_nullable_foo();
  nonnull_foo($foo);
}

class Foo {}

function nonnull_foo(Foo $foo) {}

test();
