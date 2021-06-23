@kphp_should_fail
/passed nullable null value as \$bar argument to nonnull_bar/
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // ->bar is used in the else branch.

  if ($foo->bar !== null) {
  } else {
    nonnull_bar($foo->bar);
  }

  if ($foo->bar) {
  } else {
    nonnull_bar($foo->bar);
  }
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
