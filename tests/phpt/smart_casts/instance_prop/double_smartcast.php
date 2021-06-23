@ok
<?php

class Foo {
  /** @var ?Foo */
  public $next;

  public $value = 54;
}

function nonnull_foo(Foo $foo) {}

function test_eq3(Foo $foo = null) {
  if ($foo !== null) {
    nonnull_foo($foo);
    if ($foo->next !== null) {
      nonnull_foo($foo->next);
    }
    nonnull_foo($foo);
  }
}

function test_drop_false(Foo $foo = null) {
  if ($foo) {
    nonnull_foo($foo);
    if ($foo->next) {
      nonnull_foo($foo->next);
    }
    nonnull_foo($foo);
  }
}

function test_redundant_conv(Foo $foo = null) {
  if ($foo !== null) {
    nonnull_foo($foo);
    if ($foo) {
      nonnull_foo($foo);
      if ($foo->next !== null) {
        nonnull_foo($foo->next);
        if ($foo->next) {
          nonnull_foo($foo->next);
        }
        nonnull_foo($foo->next);
      }
      nonnull_foo($foo);
    }
    nonnull_foo($foo);
  }
}

test_eq3(new Foo());
test_drop_false(new Foo());
test_redundant_conv(new Foo());
