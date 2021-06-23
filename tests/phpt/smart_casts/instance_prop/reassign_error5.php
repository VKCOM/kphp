@kphp_should_fail
/passed nullable Bar value as \$bar argument to bar_ref/
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // Passing foo->bar as ref discards smart casts.
  if ($foo->bar) {
    bar_ref($foo->bar);
    nonnull_bar($foo->bar);
  }
}

function bar_ref(Bar &$bar) {}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
