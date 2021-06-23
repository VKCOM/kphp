@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // Even though only 1 branch does the null writing,
  // smartcasts are discarded anyway.

  if ($foo->bar === null) {
    return;
  }

  if ($foo !== $foo) {
    $foo->bar = null;
  } else {
    $foo->bar = new Bar();
  }

  nonnull_bar($foo->bar);
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
