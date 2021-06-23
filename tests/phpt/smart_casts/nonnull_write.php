@ok php7_4
<?php

function new_foo(): Foo {
  return new Foo();
}

function new_nullable_foo(): ?Foo {
  return new Foo();
}

function test_assign_new(): Foo {
  $foo = null;
  $foo = new Foo();
  nonnull_foo($foo);
  return $foo;
}

function test_assign_new2(): Foo {
  /** @var ?Foo */
  $foo = null;
  if (!$foo) {
    $foo = new Foo();
    nonnull_foo($foo);
  }
  nonnull_foo($foo);
  return $foo;
}

function test_assign_new3(Foo $foo = null): Foo {
  if (!$foo) {
    $foo = new Foo();
    nonnull_foo($foo);
  }
  nonnull_foo($foo);
  return $foo;
}

function test_assign_new4(Foo $foo = null): Foo {
  if ($foo) {
    nonnull_foo($foo);
  } else {
    $foo = new Foo();
    nonnull_foo($foo);
  }
  nonnull_foo($foo);
  return $foo;
}

function test_assign_call(): Foo {
  /** @var ?Foo */
  $foo = null;
  if (!$foo) {
    $foo = new_foo();
  }
  return $foo;
}

function test_assign_various() {
  $tag = 0;

  /** @var ?Foo */
  $foo = null;

  if ($tag === 0) {
    $foo = new Foo();
  } else if ($tag === 1) {
    $foo = new_foo();
  } else {
    $x = new_nullable_foo();
    $foo = $x ? $x : new Foo();
  }

  return $foo;
}

class Foo {}

function nonnull_foo(Foo $foo) {}

test_assign_new();
test_assign_new2();
test_assign_new3();
test_assign_new4();
test_assign_call();
test_assign_various();
