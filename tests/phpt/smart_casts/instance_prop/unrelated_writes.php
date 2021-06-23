@ok php7_4
KPHP_ERROR_ON_WARNINGS=1
<?php

function test1(Foo $foo) {
  // We discard any smartcasts to foo->y,
  // but foo->x smartcast continues to work.

  if ($foo->x !== null) {
    if ($foo->y !== null) {
      $foo->y = null;
      nonnull_bar($foo->x);
    }
    nonnull_bar($foo->x);
  }
}

function test2(Foo $foo) {
  // This is a questionable code with always-false condition,
  // but foo->x null write happens before the smartcast,
  // so the type checking happily accepts this code.

  $foo->x = null;
  if ($foo->x !== null) {
    nonnull_bar($foo->x);
  }
}

function test3(Foo $foo) {
  // null is assigned in unrelated branch,
  // hence for the else branch continue to work.

  if ($foo->x === null) {
    $foo->x = null;
  } else {
    nonnull_bar($foo->x);
  }
}

function test4(Foo $foo) {
  // Just like with null assignments, passing by
  // ref in unrelated branch doesn't affect the casts.

  if ($foo->x === null) {
    bar_ref($foo->x);
  } else {
    nonnull_bar($foo->x);
  }
}

function bar_ref(?Bar &$bar) {}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $x = null;
  public ?Bar $y = null;
}

class Bar {}

test1(new Foo());
test2(new Foo());
test3(new Foo());
test4(new Foo());
