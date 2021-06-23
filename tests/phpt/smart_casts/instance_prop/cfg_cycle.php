@ok php7_4
KPHP_ERROR_ON_WARNINGS=1
<?php

function nonnull_foo(Foo $foo) {}
function nonnull_bar(Bar $bar) {}

class Foo {
  public Bar $bar;
  public ?Bar $nullable_bar;
  public ?Foo $nullable_foo;

  public function __construct(Foo $foo = null) {
    $this->bar = new Bar();
    $this->nullable_bar = new Bar();
    $this->nullable_foo = $foo;
  }
}

function test_loops(Foo $foo) {
  $cond = false;

  while ($foo->nullable_bar) {
    nonnull_bar($foo->nullable_bar);
    if (!$cond) {
      break;
    }
  }

  if ($foo->nullable_bar) {
    while ($cond) {
      for ($i = 0; $i < 5; $i++) {
        nonnull_bar($foo->nullable_bar);
      }
      nonnull_bar($foo->nullable_bar);
    }
    nonnull_bar($foo->nullable_bar);
  }

  do {
    if ($foo->nullable_foo === null) {
    } else {
      nonnull_foo($foo->nullable_foo);
      if ($foo->nullable_bar !== null) {
        nonnull_bar($foo->nullable_bar);
      }
      nonnull_foo($foo->nullable_foo);
    }
  } while ($cond);
}

function test_switches(Foo $foo) {
  $tag = 10;

  switch ($tag) {
  case 20:
    if ($foo->nullable_bar !== null) {
      nonnull_bar($foo->nullable_bar);
    }
    break;
  }

  if ($foo->nullable_bar !== null) {
    switch ($tag) {
    case 20:
      nonnull_bar($foo->nullable_bar);
      break;
    case 30:
      if ($foo->nullable_foo !== null) {
        nonnull_bar($foo->nullable_bar);
        nonnull_foo($foo->nullable_foo);
      }
      break;
    }
  }
}

class Bar {}

$foo = new Foo(new Foo(null));

test_loops($foo);
test_switches($foo);
