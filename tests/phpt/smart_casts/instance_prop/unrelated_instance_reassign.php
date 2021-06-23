@ok php7_4
KPHP_ERROR_ON_WARNINGS=1
<?php

function test1(Foo $foo) {
  // foo is reassigned in unrelated branch.
  if ($foo->x === null) {
    $foo = new Foo();
  } else {
    nonnull_bar($foo->x);
  }
}

function test2(Foo $foo) {
  if ($foo !== $foo) {
    $foo = new Foo();
    return $foo;
  }

  if ($foo->x) {
    nonnull_bar($foo->x);
  }
  if ($foo->y) {
    nonnull_bar($foo->y);
  }
  return $foo;
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $x = null;
  public ?Bar $y = null;
}

class Bar {}

test1(new Foo());
test2(new Foo());
