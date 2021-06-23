@kphp_should_fail
/passed nullable Bar value as \$bar argument to nonnull_bar/
/passed nullable Bar value as \$bar argument to nonnull_bar/
<?php

function test(Foo $foo) {
  // ->bar is used in the false branch.

  nonnull_bar(($foo->bar !== null) ? new Bar() : $foo->bar);
  nonnull_bar($foo->bar ? new Bar() : $foo->bar);
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
